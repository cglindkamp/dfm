/* See LICENSE file for copyright and license details. */
#include <errno.h>
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
#include "keymap.h"
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

static bool enter_directory(struct application *app, const char *oldpathname)
{
	while(1) {
		const char *cwd = path_tocstr(&app->cwd);

		if(app->inotify_fd != -1) {
			if(app->inotify_watch != -1)
				inotify_rm_watch(app->inotify_fd, app->inotify_watch);

			app->inotify_watch = inotify_add_watch(app->inotify_fd, cwd,
				IN_CREATE |
				IN_DELETE |
				IN_MOVED_FROM |
				IN_MOVED_TO |
				IN_MODIFY |
				IN_EXCL_UNLINK
				);
		}

		if(chdir(cwd) == 0 && dirmodel_change_directory(&app->model, cwd))
			break;

		if(strcmp(cwd, "/") == 0) {
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

	const list_t *list;
	int ret = dirmodel_getmarkedfilenames(&app->model, &list);
	if(ret == 0) {
		bool ret = dump_filelist_to_file(dir_fd, "marked", list);
		list_delete(list, free);
		if(!ret)
			goto err_dircreated;
	} else if(ret == ENOMEM) {
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
	pid_t pid;
	int status;
	bool foreground = run_in_foreground();

	if(foreground) {
		endwin();
		if(processmanager_spawn(&app->pm, path_tocstr(handler_path), args, path, foreground, &pid) == 0) {
			processmanager_waitpid(&app->pm, pid, &status);
			doupdate();
			goto success;
		} else
			doupdate();
	} else if(processmanager_spawn(&app->pm, path_tocstr(handler_path), args, path, foreground, &pid))
		goto success;

err_dircreated:
	remove(path);
success:
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

static void change_directory(struct application *app, const char *path)
{
	save_current_position(app);

	if(path_set_from_string(&app->cwd, path) == 0)
		enter_directory(app, NULL);
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
	const list_t *list = NULL;

	int ret = dirmodel_getmarkedfilenames(&app->model, &list);
	char *cwd_copy = strdup(path_tocstr(&app->cwd));

	if(ret == ENOMEM)
		goto err;

	if(ret == 0) {
		if(cwd_copy == NULL)
			goto err;
	} else if(ret == ENOENT) {
		if(listmodel_count(&app->model) == 0)
			goto err;

		list_t *selectedlist = list_new(1);
		if(selectedlist == NULL)
			goto err;
		list = selectedlist;

		size_t index = listview_getindex(&app->view);
		char *filename = strdup(dirmodel_getfilename(&app->model, index));
		if(filename == NULL)
			goto err;

		if(!list_append(selectedlist, filename)) {
			free(filename);
			goto err;
		}
	}
	clipboard_set_contents(&app->clipboard, cwd_copy, list);
	return;
err:
	free(cwd_copy);
	list_delete(list, free);
	clipboard_set_contents(&app->clipboard, NULL, NULL);
}

static void quit(struct application *app, const char *unused)
{
	(void)unused;
	app->running = false;
}

struct command_map application_command_map[] = {
	{ "navigate_up", navigate_up, false },
	{ "navigate_down", navigate_down, false },
	{ "navigate_pageup", navigate_pageup, false },
	{ "navigate_pagedown", navigate_pagedown, false },
	{ "navigate_left", navigate_left, false },
	{ "navigate_right", navigate_right, false },
	{ "mark", mark, false },
	{ "change_directory", change_directory, true },
	{ "invoke_handler", invoke_handler, true },
	{ "yank", yank, false },
	{ "quit", quit, false },
	{ NULL, NULL, false },
};

static void handle_stdin(struct application *app)
{
	wint_t key;
	int ret;

	ret = wget_wch(app->status, &key);
	if(ret == ERR)
		return;

	keymap_handlekey(app->keymap, app, key, ret == KEY_CODE_YES ? true : false);
}

static void handle_signal(struct application *app)
{
	struct signalfd_siginfo info;
	int status;

	read(app->signal_fd, &info, sizeof(info));

	switch(info.ssi_signo) {
	case SIGWINCH:
		endwin();
		doupdate();
		mvwin(app->status, LINES - 1, 0);
		wresize(app->status, 1, COLS);
		wrefresh(app->status);
		listview_resize(&app->view, COLS, LINES - 1);
	case SIGCHLD:
		if(info.ssi_code == CLD_EXITED ||
		   info.ssi_code == CLD_KILLED ||
		   info.ssi_code == CLD_DUMPED) {
			processmanager_waitpid(&app->pm, info.ssi_pid, &status);
		}
	}
}

static void handle_inotify(struct application *app)
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

void application_run(struct application *app)
{
	struct pollfd pollfds[3] = {
		{ .events = POLLIN, .fd = 0, },
		{ .events = POLLIN, .fd = app->signal_fd, },
		{ .events = POLLIN, .fd = app->inotify_fd, },
	};
	int nfds = app->inotify_fd == -1 ? 2 : 3;

	app->running = true;
	while(app->running) {
		int ret = poll(pollfds, nfds, -1);
		if(ret > 0) {
			if(pollfds[0].revents & POLLIN)
				handle_stdin(app);
			if(pollfds[1].revents & POLLIN)
				handle_signal(app);
			if(pollfds[2].revents & POLLIN)
				handle_inotify(app);
		}
	}
}


static int create_signalfd()
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGWINCH);
	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	return signalfd(-1, &sigset, SFD_CLOEXEC);
}

static bool load_keymap(struct application *app)
{
	struct path *keymap_path = determine_usable_config_file(PROJECT, NULL, "keymap", R_OK);
	if(keymap_path == NULL) {
		app->keymap = NULL;
		return false;
	}

	if(keymap_newfromfile(&app->keymap, path_tocstr(keymap_path), application_command_map) != 0) {
		app->keymap = NULL;
		return false;
	}

	return true;
}

bool application_init(struct application *app)
{
	bool ret = true;

	processmanager_init(&app->pm);
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

	if(!load_keymap(app))
		ret = false;

	if(ret == false)
		return false;

	if(!path_set_to_current_working_directory(&app->cwd))
		return false;

	if(!enter_directory(app, NULL))
		return false;

	app->signal_fd = create_signalfd();
	if(app->signal_fd == -1)
		return false;

	return true;
}

void application_destroy(struct application *app)
{
	keymap_delete(app->keymap);
	listview_destroy(&app->view);
	path_destroy(&app->cwd);
	if(app->status)
		delwin(app->status);
	dict_delete(app->stored_positions, true);
	dirmodel_destroy(&app->model);
	clipboard_destroy(&app->clipboard);
	processmanager_destroy(&app->pm);
}
