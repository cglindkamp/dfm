/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>

#include "clipboard.h"
#include "list.h"
#include "listmodel.h"
#include "listview.h"
#include "dirmodel.h"
#include "dict.h"
#include "path.h"
#include "xdg.h"

void _nc_freeall();

struct loopdata {
	struct listview view;
	struct listmodel model;
	list_t *stored_positions;
	WINDOW *status;
	struct path cwd;
	struct clipboard clipboard;
	int sigwinch_fd;
	int inotify_fd;
	int inotify_watch;
	bool running;
};

void init_ncurses()
{
	initscr();
	noecho();
	raw();
	curs_set(0);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLUE);
}

static void save_current_position(struct loopdata *data)
{
	if(listmodel_count(&data->model) != 0) {
		free(dict_get(data->stored_positions, path_tocstr(&data->cwd)));

		size_t index = listview_getindex(&data->view);
		const char *filename = dirmodel_getfilename(&data->model, index);

		char *filename_copy = strdup(filename);
		if(filename_copy == NULL)
			return;
		if(!dict_set(data->stored_positions, path_tocstr(&data->cwd), filename_copy))
			free(filename_copy);
	}
}

static void select_stored_position(struct loopdata *data, const char *oldfilename)
{
	size_t index;
	const char *filename;

	if(oldfilename)
		filename = oldfilename;
	else
		filename = dict_get(data->stored_positions, path_tocstr(&data->cwd));

	if(filename) {
		dirmodel_get_index(&data->model, filename, &index);
		if(index == listmodel_count(&data->model))
			index--;
		listview_setindex(&data->view, index);
	}
}

static void display_current_path(struct loopdata *data)
{
	wmove(data->status, 0, 0);
	wclear(data->status);
	wprintw(data->status, "%s", path_tocstr(&data->cwd));
	wrefresh(data->status);
}

static void sigwinch_cb(struct loopdata *data)
{
	struct signalfd_siginfo info;

	read(data->sigwinch_fd, &info, sizeof(info));

	endwin();
	doupdate();
	mvwin(data->status, LINES - 1, 0);
	wresize(data->status, 1, COLS);
	wrefresh(data->status);
	listview_resize(&data->view, COLS, LINES - 1);
}

static struct path *determine_usable_config_file(const char *project, const char *subdir, const char *config, int flags)
{
	struct path *path = NULL;
	list_t *list = xdg_get_config_dirs(true);

	if(list == NULL)
		return NULL;

	for(size_t i = 0; i < list_length(list); i++) {
		struct path *curpath = list_get_item(list, i);
		if(!path_add_component(curpath, project))
			goto err;
		if(subdir) {
			if(!path_add_component(curpath, subdir))
				goto err;
		}
		if(!path_add_component(curpath, config))
			goto err;
		if(path == NULL && access(path_tocstr(curpath), flags) == 0) {
			path = curpath;
			list_set_item(list, i, NULL);
		}
	}
err:
	list_delete(list, (list_item_deallocator)path_delete);
	return path;
}

static bool dump_string_to_file(int dir_fd, const char *filename, const char *value)
{
	int fd = openat(dir_fd, filename, O_CREAT|O_WRONLY, 0700);
	if(fd < 0)
		return false;

	int ret = write(fd, value, strlen(value));

	close(fd);

	return ret < 0 ? false : true;
}

static bool dump_filelist_to_file(int dir_fd, const char *filename, const list_t *list)
{
	int ret = 0;

	int fd = openat(dir_fd, filename, O_CREAT|O_WRONLY, 0700);
	if(fd < 0)
		return false;

	for(size_t i = 0; i < list_length(list); i++) {
		const char *file = list_get_item(list, i);
		if((ret = write(fd, file, strlen(file)) < 0))
			break;
		if((ret = write(fd, "\0", 1)) < 0)
			break;
	}
	close(fd);

	return ret < 0 ? false : true;
}

static bool run_in_foreground()
{
	const char *display = getenv("DISPLAY");

	if(display == NULL || strlen(display) == 0)
		return true;

	const char *foreground = getenv("FILES_FOREGROUND");

	if(foreground != NULL && strcmp(foreground, "yes") == 0)
		return true;

	return false;
}

static bool spawn(const char *cwd, const char *program, char * const argv[])
{
	bool foreground = run_in_foreground();

	if(foreground)
		endwin();

	int pid = fork();

	if(pid < 0)
		return false;
	if(pid > 0) {
		if(foreground) {
			waitpid(pid, NULL, 0);
			doupdate();
		}
		return true;
	}

	if(!foreground) {
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
	}
	chdir(cwd);
	execvp(program, argv);
	exit(EXIT_FAILURE);
}

static void invoke_handler(struct loopdata *data, const char *handler_name)
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

	if(listmodel_count(&data->model) > 0) {
		size_t index = listview_getindex(&data->view);
		const char *selected = dirmodel_getfilename(&data->model, index);
		if(!dump_string_to_file(dir_fd, "selected", selected))
			goto err_dircreated;
	}

	const list_t *list = dirmodel_getmarkedfilenames(&data->model);
	if(list != NULL) {
		bool ret = dump_filelist_to_file(dir_fd, "marked", list);
		list_delete(list, free);
		if(!ret)
			goto err_dircreated;
	}

	if(!dump_string_to_file(dir_fd, "cwd", path_tocstr(&data->cwd)))
		goto err_dircreated;

	const char *clipboard_path = clipboard_get_path(&data->clipboard);
	if(clipboard_path)
		if(!dump_string_to_file(dir_fd, "clipboard_path", clipboard_path))
			goto err_dircreated;

	list = clipboard_get_filelist(&data->clipboard);
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
	if(spawn(path_tocstr(&data->cwd), path_tocstr(handler_path), args))
		goto succes;

err_dircreated:
	remove(path);
succes:
	close(dir_fd);
err_dir:
	path_delete(handler_path);
}

void navigate_up(struct loopdata *data, const char *unused)
{
	(void)unused;
	listview_up(&data->view);
}

void navigate_down(struct loopdata *data, const char *unused)
{
	(void)unused;
	listview_down(&data->view);
}

void navigate_pageup(struct loopdata *data, const char *unused)
{
	(void)unused;
	listview_pageup(&data->view);
}

void navigate_pagedown(struct loopdata *data, const char *unused)
{
	(void)unused;
	listview_pagedown(&data->view);
}

static bool enter_directory(struct loopdata *data, const char *oldpathname)
{
	while(1) {
		if(data->inotify_fd != -1) {
			if(data->inotify_watch != -1)
				inotify_rm_watch(data->inotify_fd, data->inotify_watch);

			data->inotify_watch = inotify_add_watch(data->inotify_fd, path_tocstr(&data->cwd),
				IN_CREATE |
				IN_DELETE |
				IN_MOVED_FROM |
				IN_MOVED_TO |
				IN_MODIFY |
				IN_EXCL_UNLINK
				);
		}

		if(dirmodel_change_directory(&data->model, path_tocstr(&data->cwd)))
			break;

		if(strcmp(path_tocstr(&data->cwd), "/") == 0) {
			puts("Cannot even open \"/\", exiting");
			data->running = false;
			return false;
		}
		path_remove_component(&data->cwd, &oldpathname);
	}

	select_stored_position(data, oldpathname);
	display_current_path(data);

	return true;
}

static void change_directory(struct loopdata *data, const char *path)
{
	save_current_position(data);

	if(path_set_from_string(&data->cwd, path) == 0)
		enter_directory(data, NULL);
}

void navigate_left(struct loopdata *data, const char *unused)
{
	(void)unused;
	const char *oldpathname = NULL;

	save_current_position(data);

	if(path_remove_component(&data->cwd, &oldpathname))
		enter_directory(data, oldpathname);
}

void navigate_right(struct loopdata *data, const char *unused)
{
	(void)unused;

	if(listmodel_count(&data->model) == 0)
		return;

	save_current_position(data);

	size_t index = listview_getindex(&data->view);
	if(dirmodel_isdir(&data->model, index)) {
		const char *filename = dirmodel_getfilename(&data->model, index);
		if(!path_add_component(&data->cwd, filename))
			return;
		enter_directory(data, NULL);
	} else {
		invoke_handler(data, "open");
	}
}

void mark(struct loopdata *data, const char *unused)
{
	(void)unused;
	size_t index = listview_getindex(&data->view);
	listmodel_setmark(&data->model, index, !listmodel_ismarked(&data->model, index));
	listview_down(&data->view);
}

void yank(struct loopdata *data, const char *unused)
{
	(void)unused;
	const list_t *list = dirmodel_getmarkedfilenames(&data->model);
	if(list) {
		char *cwd_copy = strdup(path_tocstr(&data->cwd));
		if(cwd_copy == NULL) {
			list_delete(list, free);
			clipboard_set_contents(&data->clipboard, NULL, NULL);
		} else
			clipboard_set_contents(&data->clipboard, cwd_copy, list);
	}
}

void quit(struct loopdata *data, const char *unused)
{
	(void)unused;
	data->running = false;
}

struct keyspec {
	int type;
	wint_t key;
};

struct keymap {
	struct keyspec keyspec;
	void (*cmd)(struct loopdata *, const char *);
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

static void stdin_cb(struct loopdata *data)
{
	wint_t key;
	int ret;

	ret = wget_wch(data->status, &key);
	if(ret == ERR)
		return;

	for(size_t i = 0; i < sizeof(keymap)/sizeof(keymap[0]); i++) {
		if(keymap[i].keyspec.type == ret && keymap[i].keyspec.key == key) {
			keymap[i].cmd(data, keymap[i].param);
			break;
		}
	}
}

static void inotify_cb(struct loopdata *data)
{
	char buf[4096]
		__attribute__((aligned(__alignof(struct inotify_event))));
	char *ptr;
	const struct inotify_event *event;
	ssize_t len;

	len = read(data->inotify_fd, &buf, sizeof(buf));
	if(len <= 0)
		return;

	for(ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
		event = (const struct inotify_event *)ptr;

		if(event->wd != data->inotify_watch)
			continue;

		if(event->mask & (IN_DELETE | IN_MOVED_FROM)) {
			dirmodel_notify_file_deleted(&data->model, event->name);
		} else if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) {
			(void)dirmodel_notify_file_added_or_changed(&data->model, event->name);
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

int main(void)
{
	struct loopdata data;
	int ret = 0;

	setlocale(LC_ALL, "");

	if(!path_init(&data.cwd, PATH_MAX)) {
		puts("Cannot get current working directory, exiting!");
		ret = -1;
		goto err_path;
	}

	if(!path_set_to_current_working_directory(&data.cwd)) {
		puts("Cannot get current working directory, exiting!");
		ret = -1;
		goto err_path_cwd;
	}

	data.inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	data.inotify_watch = -1;

	init_ncurses();

	data.status = newwin(1, COLS, LINES - 1, 0);
	if(data.status == NULL) {
		endwin();
		puts("Cannot create status line, exiting!");
		ret = -1;
		goto err_status_line;
	}

	data.stored_positions = dict_new();
	if(data.stored_positions == NULL) {
		endwin();
		puts("Cannot allocate stored positions, exiting!");
		ret = -1;
		goto err_stored_positions;
	}

	keypad(data.status, TRUE);
	nodelay(data.status, TRUE);

	clipboard_init(&data.clipboard);

	dirmodel_init(&data.model);
	if(!enter_directory(&data, NULL)) {
		ret = -1;
		goto err_dirmodel;
	}

	if(!listview_init(&data.view, &data.model, 0, 0, COLS, LINES - 1))
	{
		endwin();
		puts("Cannot create list view, exiting!");
		ret = -1;
		goto err_listview;
	};

	data.sigwinch_fd = create_sigwinch_signalfd();
	if(data.sigwinch_fd == -1) {
		endwin();
		puts("Cannot create signalfd, exiting!");
		ret = -1;
		goto err_signalfd;
	};

	struct pollfd pollfds[3] = {
		{ .events = POLLIN, .fd = 0, },
		{ .events = POLLIN, .fd = data.sigwinch_fd, },
		{ .events = POLLIN, .fd = data.inotify_fd, },
	};
	int nfds = data.inotify_fd == -1 ? 2 : 3;

	data.running = true;
	while(data.running) {
		int ret = poll(pollfds, nfds, -1);
		if(ret > 0) {
			if(pollfds[0].revents & POLLIN)
				stdin_cb(&data);
			if(pollfds[1].revents & POLLIN)
				sigwinch_cb(&data);
			if(pollfds[2].revents & POLLIN)
				inotify_cb(&data);
		}
	}

	endwin();
err_signalfd:
	listview_destroy(&data.view);
err_listview:
	dirmodel_destroy(&data.model);
err_dirmodel:
	clipboard_destroy(&data.clipboard);
	dict_delete(data.stored_positions, true);
err_stored_positions:
	delwin(data.status);
err_status_line:
	_nc_freeall();
err_path_cwd:
	path_destroy(&data.cwd);
err_path:
	return ret;
}
