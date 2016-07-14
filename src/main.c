#define _XOPEN_SOURCE_EXTENDED
#include <stdbool.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>

#include <ev.h>

#include "listmodel.h"
#include "listview.h"
#include "dirmodel.h"

struct loopdata {
	struct listview view;
	struct listmodel model;
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

bool get_current_working_directory(struct loopdata *data)
{
	data->cwdsize = PATH_MAX;
	data->cwd = malloc(data->cwdsize);
	while(getcwd(data->cwd, data->cwdsize) == NULL)
	{
		if(errno == ERANGE) {
			data->cwdsize *= 2 ;
			data->cwd = realloc(data->cwd, data->cwdsize);
		} else {
			free(data->cwd);
			return false;
		}
	}
	return true;
}

static bool remove_path_component(struct loopdata *data)
{
	if(strlen(data->cwd) == 1)
		return false;

	char *slash = rindex(data->cwd, '/');
	if(slash == data->cwd)
		*(slash + 1) = 0;
	else
		*slash = 0;
	return true;
}

static void add_path_component(struct loopdata *data, const char *filename)
{
	while(strlen(data->cwd) + strlen(filename) + 2 > data->cwdsize) {
		data->cwdsize *= 2 ;
		data->cwd = realloc(data->cwd, data->cwdsize);
	}
	if(strlen(data->cwd) != 1)
		strcat(data->cwd, "/");
	strcat(data->cwd, filename);
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
			if(remove_path_component(data))
				while(!dirmodel_change_directory(&data->model, data->cwd))
					remove_path_component(data);
			display_current_path(data);
			break;
		case KEY_RIGHT:
			if(listmodel_count(&data->model) == 0)
				break;
			size_t index = listview_getindex(&data->view);
			if(dirmodel_isdir(&data->model, index)) {
				const char *filename = dirmodel_getfilename(&data->model, index);
				add_path_component(data, filename);
				while(!dirmodel_change_directory(&data->model, data->cwd))
					remove_path_component(data);
				display_current_path(data);
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
	free(data.cwd);
	endwin();

	return 0;
}
