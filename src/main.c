/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>

#include <ev.h>

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
	struct ev_loop *loop;
	struct listview view;
	struct listmodel model;
	list_t *stored_positions;
	WINDOW *status;
	struct path cwd;
	struct clipboard clipboard;
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

static void sigwinch_cb(EV_P_ ev_signal *w, int revents)
{
	(void)w;
	(void)revents;

	struct loopdata *data = ev_userdata(EV_A);

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
	list_free(list, (list_item_deallocator)path_free_heap_allocated);
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
		list_free(list, free);
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
	path_free_heap_allocated(handler_path);
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

void navigate_left(struct loopdata *data, const char *unused)
{
	(void)unused;
	const char *oldpathname = NULL;

	save_current_position(data);

	if(path_remove_component(&data->cwd, &oldpathname)) {
		while(!dirmodel_change_directory(&data->model, path_tocstr(&data->cwd))) {
			if(strcmp(path_tocstr(&data->cwd), "/") == 0) {
				puts("Cannot even open \"/\", exiting");
				ev_break(data->loop, EVBREAK_ONE);
			}
			path_remove_component(&data->cwd, &oldpathname);
		}

		select_stored_position(data, oldpathname);
		display_current_path(data);
	}
}

void navigate_right(struct loopdata *data, const char *unused)
{
	(void)unused;
	const char *oldpathname = NULL;

	if(listmodel_count(&data->model) == 0)
		return;

	save_current_position(data);

	size_t index = listview_getindex(&data->view);
	if(dirmodel_isdir(&data->model, index)) {
		const char *filename = dirmodel_getfilename(&data->model, index);
		if(!path_add_component(&data->cwd, filename))
			return;
		while(!dirmodel_change_directory(&data->model, path_tocstr(&data->cwd))) {
			if(strcmp(path_tocstr(&data->cwd), "/") == 0) {
				puts("Cannot even open \"/\", exiting");
				ev_break(data->loop, EVBREAK_ONE);
			}
			path_remove_component(&data->cwd, &oldpathname);
		}

		select_stored_position(data, oldpathname);
		display_current_path(data);
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
			list_free(list, free);
			clipboard_set_contents(&data->clipboard, NULL, NULL);
		} else
			clipboard_set_contents(&data->clipboard, cwd_copy, list);
	}
}

void quit(struct loopdata *data, const char *unused)
{
	(void)unused;
	ev_break(data->loop, EVBREAK_ONE);
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
	{ { OK, L'D' }, invoke_handler, "delete" },
	{ { OK, L'p' }, invoke_handler, "copy" },
	{ { OK, L'P' }, invoke_handler, "move" },
	{ { OK, L's' }, invoke_handler, "shell" },
	{ { OK, L'y' }, yank, NULL },
	{ { OK, L'q' }, quit, NULL },
};

static void stdin_cb(EV_P_ ev_io *w, int revents)
{
	(void)w;
	(void)revents;

	struct loopdata *data = ev_userdata(EV_A);
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

int main(void)
{
	struct loopdata data;
	data.loop = EV_DEFAULT;
	ev_io stdin_watcher;
	ev_signal sigwinch_watcher;
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

	dirmodel_init(&data.model);
	if(!dirmodel_change_directory(&data.model, path_tocstr(&data.cwd))) {
		puts("Cannot open current working directory, exiting!");
		ret = -1;
		goto err_dirmodel;
	}

	init_ncurses();

	if(!listview_init(&data.view, &data.model, 0, 0, COLS, LINES - 1))
	{
		endwin();
		puts("Cannot create list view, exiting!");
		ret = -1;
		goto err_listview;
	};

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

	display_current_path(&data);


	ev_set_userdata(data.loop, &data);

	ev_io_init(&stdin_watcher, stdin_cb, 0, EV_READ);
	ev_io_start(data.loop, &stdin_watcher);

	ev_signal_init(&sigwinch_watcher, sigwinch_cb, SIGWINCH);
	ev_signal_start(data.loop, &sigwinch_watcher);

	ev_run(data.loop, 0);


	endwin();
	clipboard_free(&data.clipboard);
	dict_free(data.stored_positions, true);
err_stored_positions:
	delwin(data.status);
err_status_line:
	listview_free(&data.view);
err_listview:
	_nc_freeall();
err_dirmodel:
	dirmodel_free(&data.model);
err_path_cwd:
	path_free(&data.cwd);
err_path:
	ev_loop_destroy(EV_DEFAULT);

	return ret;
}
