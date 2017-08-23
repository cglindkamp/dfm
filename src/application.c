/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <limits.h>
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include "application.h"
#include "dirmodel.h"
#include "dict.h"
#include "util.h"

static void save_current_position(struct application *app)
{
	if(listmodel_count(&app->model) != 0) {
		free(dict_get(app->stored_positions, path_tocstr(&app->cwd)));

		size_t index = listview_getindex(&app->view);
		const char *filename = dirmodel_getfilename(&app->model, index);

		char *filename_copy = strdup(filename);
		if(filename_copy == NULL)
			return;
		if(!dict_set(app->stored_positions, path_tocstr(&app->cwd), filename_copy))
			free(filename_copy);
	}
}

static void select_stored_position(struct application *app, const char *oldfilename)
{
	size_t index;
	const char *filename;

	if(oldfilename)
		filename = oldfilename;
	else
		filename = dict_get(app->stored_positions, path_tocstr(&app->cwd));

	if(filename) {
		dirmodel_get_index(&app->model, filename, &index);
		if(index == listmodel_count(&app->model))
			index--;
		listview_setindex(&app->view, index);
	}
}

static void display_current_path(struct application *app)
{
	wmove(app->status, 0, 0);
	wclear(app->status);
	wprintw(app->status, "%s", path_tocstr(&app->cwd));
	wrefresh(app->status);
}

static void sigwinch_cb(struct application *app)
{
	struct signalfd_siginfo info;

	read(app->sigwinch_fd, &info, sizeof(info));

	endwin();
	doupdate();
	mvwin(app->status, LINES - 1, 0);
	wresize(app->status, 1, COLS);
	wrefresh(app->status);
	listview_resize(&app->view, COLS, LINES - 1);
}

static void invoke_handler(struct application *app, const char *handler_name)
{
	struct path *handler_path = determine_usable_config_file(PROJECT, "handlers", handler_name, X_OK);
	if(handler_path == NULL)
		return;

	char path_template[] = "/tmp/"PROJECT".XXXXXX";
	char *path = mkdtemp(path_template);
	if(path == NULL)
		goto err_dir;

	int dir_fd = open(path, O_RDONLY);
	if(dir_fd < 0)
		goto err_dircreated;

	if(listmodel_count(&app->model) > 0) {
		size_t index = listview_getindex(&app->view);
		const char *selected = dirmodel_getfilename(&app->model, index);
		if(!dump_string_to_file(dir_fd, "selected", selected))
			goto err_dircreated;
	}

	const list_t *list = dirmodel_getmarkedfilenames(&app->model);
	if(list != NULL) {
		bool ret = dump_filelist_to_file(dir_fd, "marked", list);
		list_delete(list, free);
		if(!ret)
			goto err_dircreated;
	}

	if(!dump_string_to_file(dir_fd, "cwd", path_tocstr(&app->cwd)))
		goto err_dircreated;

	const char *clipboard_path = clipboard_get_path(&app->clipboard);
	if(clipboard_path)
		if(!dump_string_to_file(dir_fd, "clipboard_path", clipboard_path))
			goto err_dircreated;

	list = clipboard_get_filelist(&app->clipboard);
	if(list != NULL) {
		bool ret = dump_filelist_to_file(dir_fd, "clipboard_list", list);
		if(!ret)
			goto err_dircreated;
	}

	char * const args[] = {
		"open",
		path,
		NULL
	};
	if(spawn(path_tocstr(&app->cwd), path_tocstr(handler_path), args))
		goto succes;

err_dircreated:
	remove(path);
succes:
	close(dir_fd);
err_dir:
	path_delete(handler_path);
}

static void navigate_up(struct application *app, const char *unused)
{
	(void)unused;
	listview_up(&app->view);
}

static void navigate_down(struct application *app, const char *unused)
{
	(void)unused;
	listview_down(&app->view);
}

static void navigate_pageup(struct application *app, const char *unused)
{
	(void)unused;
	listview_pageup(&app->view);
}

static void navigate_pagedown(struct application *app, const char *unused)
{
	(void)unused;
	listview_pagedown(&app->view);
}

static bool enter_directory(struct application *app, const char *oldpathname)
{
	while(1) {
		if(app->inotify_fd != -1) {
			if(app->inotify_watch != -1)
				inotify_rm_watch(app->inotify_fd, app->inotify_watch);

			app->inotify_watch = inotify_add_watch(app->inotify_fd, path_tocstr(&app->cwd),
				IN_CREATE |
				IN_DELETE |
				IN_MOVED_FROM |
				IN_MOVED_TO |
				IN_MODIFY |
				IN_EXCL_UNLINK
				);
		}

		if(dirmodel_change_directory(&app->model, path_tocstr(&app->cwd)))
			break;

		if(strcmp(path_tocstr(&app->cwd), "/") == 0) {
			puts("Cannot even open \"/\", exiting");
			app->running = false;
			return false;
		}
		path_remove_component(&app->cwd, &oldpathname);
	}

	select_stored_position(app, oldpathname);
	display_current_path(app);

	return true;
}

static void change_directory(struct application *app, const char *path)
{
	save_current_position(app);

	if(path_set_from_string(&app->cwd, path) == 0)
		enter_directory(app, NULL);
}

static void navigate_left(struct application *app, const char *unused)
{
	(void)unused;
	const char *oldpathname = NULL;

	save_current_position(app);

	if(path_remove_component(&app->cwd, &oldpathname))
		enter_directory(app, oldpathname);
}

static void navigate_right(struct application *app, const char *unused)
{
	(void)unused;

	if(listmodel_count(&app->model) == 0)
		return;

	save_current_position(app);

	size_t index = listview_getindex(&app->view);
	if(dirmodel_isdir(&app->model, index)) {
		const char *filename = dirmodel_getfilename(&app->model, index);
		if(!path_add_component(&app->cwd, filename))
			return;
		enter_directory(app, NULL);
	} else {
		invoke_handler(app, "open");
	}
}

static void mark(struct application *app, const char *unused)
{
	(void)unused;
	size_t index = listview_getindex(&app->view);
	listmodel_setmark(&app->model, index, !listmodel_ismarked(&app->model, index));
	listview_down(&app->view);
}

static void yank(struct application *app, const char *unused)
{
	(void)unused;
	const list_t *list = dirmodel_getmarkedfilenames(&app->model);
	if(list) {
		char *cwd_copy = strdup(path_tocstr(&app->cwd));
		if(cwd_copy == NULL) {
			list_delete(list, free);
			clipboard_set_contents(&app->clipboard, NULL, NULL);
		} else
			clipboard_set_contents(&app->clipboard, cwd_copy, list);
	}
}

static void quit(struct application *app, const char *unused)
{
	(void)unused;
	app->running = false;
}

struct keyspec {
	int type;
	wint_t key;
};

static struct keymap {
	struct keyspec keyspec;
	void (*cmd)(struct application *, const char *);
	const char *param;
} keymap[] = {
	{ { KEY_CODE_YES, KEY_UP }, navigate_up, NULL },
	{ { KEY_CODE_YES, KEY_DOWN }, navigate_down, NULL },
	{ { KEY_CODE_YES, KEY_PPAGE }, navigate_pageup, NULL },
	{ { KEY_CODE_YES, KEY_NPAGE }, navigate_pagedown, NULL },
	{ { KEY_CODE_YES, KEY_LEFT }, navigate_left, NULL },
	{ { KEY_CODE_YES, KEY_RIGHT }, navigate_right, NULL },
	{ { OK, L' ' }, mark, NULL },
	{ { OK, L'1' }, change_directory, "~" },
	{ { OK, L'2' }, change_directory, "/" },
	{ { OK, L'D' }, invoke_handler, "delete" },
	{ { OK, L'p' }, invoke_handler, "copy" },
	{ { OK, L'P' }, invoke_handler, "move" },
	{ { OK, L's' }, invoke_handler, "shell" },
	{ { OK, L'y' }, yank, NULL },
	{ { OK, L'q' }, quit, NULL },
};

static void stdin_cb(struct application *app)
{
	wint_t key;
	int ret;

	ret = wget_wch(app->status, &key);
	if(ret == ERR)
		return;

	for(size_t i = 0; i < sizeof(keymap)/sizeof(keymap[0]); i++) {
		if(keymap[i].keyspec.type == ret && keymap[i].keyspec.key == key) {
			keymap[i].cmd(app, keymap[i].param);
			break;
		}
	}
}

static void inotify_cb(struct application *app)
{
	char buf[4096]
		__attribute__((aligned(__alignof(struct inotify_event))));
	char *ptr;
	const struct inotify_event *event;
	ssize_t len;

	len = read(app->inotify_fd, &buf, sizeof(buf));
	if(len <= 0)
		return;

	for(ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
		event = (const struct inotify_event *)ptr;

		if(event->wd != app->inotify_watch)
			continue;

		if(event->mask & (IN_DELETE | IN_MOVED_FROM)) {
			dirmodel_notify_file_deleted(&app->model, event->name);
		} else if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) {
			(void)dirmodel_notify_file_added_or_changed(&app->model, event->name);
		}
	}
}

static int create_sigwinch_signalfd()
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGWINCH);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	return signalfd(-1, &sigset, SFD_CLOEXEC);
}

bool application_init(struct application *app)
{
	bool ret = true;

	clipboard_init(&app->clipboard);

	app->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	app->inotify_watch = -1;

	dirmodel_init(&app->model);

	app->stored_positions = dict_new();
	if(app->stored_positions == NULL)
		ret = false;

	app->status = newwin(1, COLS, LINES - 1, 0);
	if(app->status != NULL) {
		keypad(app->status, TRUE);
		nodelay(app->status, TRUE);
	} else
		ret = false;

	if(!path_init(&app->cwd, PATH_MAX))
		ret = false;

	if(!listview_init(&app->view, &app->model, 0, 0, COLS, LINES - 1))
		ret = false;

	if(ret == false)
		return false;

	if(!path_set_to_current_working_directory(&app->cwd))
		return false;

	if(!enter_directory(app, NULL))
		return false;

	app->sigwinch_fd = create_sigwinch_signalfd();
	if(app->sigwinch_fd == -1)
		return false;

	return true;
}

void application_destroy(struct application *app)
{
	listview_destroy(&app->view);
	path_destroy(&app->cwd);
	if(app->status)
		delwin(app->status);
	dict_delete(app->stored_positions, true);
	dirmodel_destroy(&app->model);
	clipboard_destroy(&app->clipboard);
}

void application_run(struct application *app)
{
	struct pollfd pollfds[3] = {
		{ .events = POLLIN, .fd = 0, },
		{ .events = POLLIN, .fd = app->sigwinch_fd, },
		{ .events = POLLIN, .fd = app->inotify_fd, },
	};
	int nfds = app->inotify_fd == -1 ? 2 : 3;

	app->running = true;
	while(app->running) {
		int ret = poll(pollfds, nfds, -1);
		if(ret > 0) {
			if(pollfds[0].revents & POLLIN)
				stdin_cb(app);
			if(pollfds[1].revents & POLLIN)
				sigwinch_cb(app);
			if(pollfds[2].revents & POLLIN)
				inotify_cb(app);
		}
	}
}

