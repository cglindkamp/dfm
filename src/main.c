#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include <ev.h>

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

		dict_set(data->stored_positions, path_tocstr(&data->cwd), strdup(filename));
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

static struct path *determine_usable_config_file(const char *project, const char *config, int flags)
{
	struct path *path = NULL;
	list_t *list = xdg_get_config_dirs(true);

	for(size_t i = 0; i < list_length(list); i++) {
		struct path *curpath = list_get_item(list, i);
		path_add_component(curpath, project);
		path_add_component(curpath, config);
		if(path == NULL && access(path_tocstr(curpath), flags) == 0)
			path = curpath;
		else
			path_free_heap_allocated(curpath);
	}
	list_free(list, NULL);
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

static bool dump_filelist_to_file(int dir_fd, const char *filename, list_t *list)
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

static bool spawn(const char *cwd, const char *program, char * const argv[])
{
	int pid = fork();

	if(pid < 0)
		return false;
	if(pid > 0)
		return true;

	int fd = open("/dev/null", O_RDWR);

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	chdir(cwd);
	execvp(program, argv);
	exit(EXIT_FAILURE);
}

static void invoke_handler(struct loopdata *data, const char *handler_name)
{
	struct path *handler_path = determine_usable_config_file(PROJECT, handler_name, X_OK);
	if(handler_name == NULL)
		return;

	char path_template[] = "/tmp/"PROJECT".XXXXXX";
	char *path = mkdtemp(path_template);
	if(path == NULL)
		goto err_dir;

	int dir_fd = open(path, O_RDONLY);
	if(dir_fd < 0)
		goto err_dircreated;

	size_t index = listview_getindex(&data->view);
	const char *selected = dirmodel_getfilename(&data->model, index);
	if(!dump_string_to_file(dir_fd, "selected", selected))
		goto err_dircreated;

	list_t *list = dirmodel_getmarkedfilenames(&data->model);
	if(list != NULL) {
		bool ret = dump_filelist_to_file(dir_fd, "marked", list);
		list_free(list, free);
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

static void stdin_cb(EV_P_ ev_io *w, int revents)
{
	(void)w;
	(void)revents;

	struct loopdata *data = ev_userdata(EV_A);
	wint_t key;
	int ret;
	const char *oldpathname = NULL;

	ret = wget_wch(data->status, &key);
	if(ret == ERR)
		return;

	if(ret == KEY_CODE_YES) {
		switch(key) {
		case KEY_UP:
			listview_up(&data->view);
			break;
		case KEY_DOWN:
			listview_down(&data->view);
			break;
		case KEY_PPAGE:
			listview_pageup(&data->view);
			break;
		case KEY_NPAGE:
			listview_pagedown(&data->view);
			break;
		case KEY_LEFT:
			save_current_position(data);

			if(path_remove_component(&data->cwd, &oldpathname)) {
				while(!dirmodel_change_directory(&data->model, path_tocstr(&data->cwd)))
					path_remove_component(&data->cwd, &oldpathname);

				select_stored_position(data, oldpathname);
				display_current_path(data);
			}

			break;
		case KEY_RIGHT:
			if(listmodel_count(&data->model) == 0)
				break;

			save_current_position(data);

			size_t index = listview_getindex(&data->view);
			if(dirmodel_isdir(&data->model, index)) {
				const char *filename = dirmodel_getfilename(&data->model, index);
				path_add_component(&data->cwd, filename);
				while(!dirmodel_change_directory(&data->model, path_tocstr(&data->cwd)))
					path_remove_component(&data->cwd, &oldpathname);

				select_stored_position(data, oldpathname);
				display_current_path(data);
			} else {
				invoke_handler(data, "open");
			}
			break;
		}
	} else {
		switch(key) {
		case L' ':
			{
				size_t index = listview_getindex(&data->view);
				listmodel_setmark(&data->model, index, !listmodel_ismarked(&data->model, index));
				break;
			}
		}
	}
	if(ret == OK && key == 3) // ^C
		ev_break(EV_A_ EVBREAK_ONE);
}

int main(void)
{
	struct loopdata data;
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	ev_signal sigwinch_watcher;

	setlocale(LC_ALL, "");

	path_init(&data.cwd, PATH_MAX);
	if(!path_set_to_current_working_directory(&data.cwd)) {
		puts("Cannot get current working directory, exiting!");
		path_free(&data.cwd);
		return -1;
	}

	dirmodel_init(&data.model);
	if(!dirmodel_change_directory(&data.model, path_tocstr(&data.cwd))) {
		puts("Cannot open current working directory, exiting!");
		dirmodel_free(&data.model);
		path_free(&data.cwd);
		return -1;
	}

	init_ncurses();

	data.stored_positions = dict_new();
	data.status = newwin(1, COLS, LINES - 1, 0);
	keypad(data.status, TRUE);
	nodelay(data.status, TRUE);

	display_current_path(&data);

	listview_init(&data.view, &data.model, 0, 0, COLS, LINES - 1);

	ev_set_userdata(loop, &data);

	ev_io_init(&stdin_watcher, stdin_cb, 0, EV_READ);
	ev_io_start(loop, &stdin_watcher);

	ev_signal_init(&sigwinch_watcher, sigwinch_cb, SIGWINCH);
	ev_signal_start(loop, &sigwinch_watcher);

	ev_run(loop, 0);

	listview_free(&data.view);
	dirmodel_free(&data.model);
	dict_free(data.stored_positions, true);
	path_free(&data.cwd);
	endwin();
       _nc_freeall();
       ev_loop_destroy(EV_DEFAULT);

	return 0;
}
