#define _XOPEN_SOURCE_EXTENDED
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include <ev.h>

#include "listmodel.h"
#include "listview.h"
#include "dirmodel.h"
#include "dict.h"

struct loopdata {
	struct listview view;
	struct listmodel model;
	list_t *stored_positions;
	WINDOW *status;
	char *cwd;
	size_t cwdsize;
};

void init_ncurses()
{
	initscr();
	noecho();
	raw();
	curs_set(0);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
}

static void cwd_make_room(struct loopdata *data)
{
	data->cwdsize *= 2 ;
	data->cwd = realloc(data->cwd, data->cwdsize);
}

bool get_current_working_directory(struct loopdata *data)
{
	data->cwdsize = PATH_MAX;
	data->cwd = malloc(data->cwdsize);
	while(getcwd(data->cwd, data->cwdsize) == NULL)
	{
		if(errno == ERANGE) {
			cwd_make_room(data);
		} else {
			free(data->cwd);
			return false;
		}
	}
	return true;
}

static bool remove_path_component(struct loopdata *data, char **removed_component)
{
	if(strlen(data->cwd) == 1)
		return false;

	char *slash = rindex(data->cwd, '/');

	if(removed_component) {
		*removed_component = slash + 1;
		if(slash == data->cwd) {
			size_t component_length = strlen(data->cwd + 1);
			if(component_length + 3 > data->cwdsize)
				cwd_make_room(data);
			memmove(data->cwd + 2, data->cwd + 1, component_length + 1);
			(*removed_component)++;
		}
	}

	if(slash == data->cwd)
		*(slash + 1) = 0;
	else
		*slash = 0;
	return true;
}

static void add_path_component(struct loopdata *data, const char *filename)
{
	while(strlen(data->cwd) + strlen(filename) + 2 > data->cwdsize)
		cwd_make_room(data);
	if(strlen(data->cwd) != 1)
		strcat(data->cwd, "/");
	strcat(data->cwd, filename);
}

static void save_current_position(struct loopdata *data)
{
	if(listmodel_count(&data->model) != 0) {
		free(dict_get(data->stored_positions, data->cwd));

		size_t index = listview_getindex(&data->view);
		const char *filename = dirmodel_getfilename(&data->model, index);

		dict_set(data->stored_positions, data->cwd, strdup(filename));
	}
}

static void select_stored_position(struct loopdata *data, const char *oldfilename)
{
	size_t index;
	const char *filename;

	if(oldfilename)
		filename = oldfilename;
	else
		filename = dict_get(data->stored_positions, data->cwd);

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
	wprintw(data->status, "%s", data->cwd);
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

static void spawn(const char *cwd, const char *program, char * const argv[])
{
	int pid = fork();

	if(pid != 0)
		return;

	int fd = open("/dev/null", O_RDWR);

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	chdir(cwd);
	execvp(program, argv);
}

static void open_file(const char *cwd, const char *filename)
{
	char * const args[] = {
		"xdg-open",
		(char *)filename,
		NULL
	};
	spawn(cwd, "xdg-open", args);
}

static void stdin_cb(EV_P_ ev_io *w, int revents)
{
	(void)w;
	(void)revents;

	struct loopdata *data = ev_userdata(EV_A);
	wint_t key;
	int ret;
	char *oldpathname = NULL;

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

			if(remove_path_component(data, &oldpathname)) {
				while(!dirmodel_change_directory(&data->model, data->cwd))
					remove_path_component(data, &oldpathname);

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
				add_path_component(data, filename);
				while(!dirmodel_change_directory(&data->model, data->cwd))
					remove_path_component(data, &oldpathname);

				select_stored_position(data, oldpathname);
				display_current_path(data);
			} else {
				const char *filename = dirmodel_getfilename(&data->model, index);
				open_file(data->cwd, filename);
			}
			break;
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

	if(!get_current_working_directory(&data)) {
		puts("Cannot get current working directory, exiting!");
		return -1;
	}

	dirmodel_init(&data.model);
	if(!dirmodel_change_directory(&data.model, data.cwd)) {
		puts("Cannot open current working directory, exiting!");
		dirmodel_free(&data.model);
		free(data.cwd);
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
	free(data.cwd);
	endwin();

	return 0;
}
