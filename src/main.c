#define _XOPEN_SOURCE_EXTENDED
#include <stdbool.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <ev.h>

#include "listmodel.h"
#include "listview.h"
#include "dirmodel.h"

struct loopdata {
	struct listview view;
	struct listmodel model;
	WINDOW *status;
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

static void sigwinch_cb(EV_P_ ev_signal *w, int revents)
{
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
	struct loopdata *data = ev_userdata(EV_A);
	wint_t key;
	int ret;
	char cwd[PATH_MAX];
	char *currentfile;

	ret = wget_wch(data->status, &key);
	if(ret == ERR)
		return;

	wmove(data->status, 0, 0);
	wprintw(data->status, "%-3d %-10d", ret, key);
	wrefresh(data->status);

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
			dirmodel_free(&data->model);
			chdir("..");
			dirmodel_init(&data->model, getcwd(cwd, PATH_MAX));
			listview_setmodel(&data->view, &data->model);
			break;
		case KEY_RIGHT:
			currentfile = strdup(dirmodel_getfilename(&data->model, listview_getindex(&data->view)));
			dirmodel_free(&data->model);
			chdir(currentfile);
			dirmodel_init(&data->model, getcwd(cwd, PATH_MAX));
			listview_setmodel(&data->view, &data->model);
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
	char cwd[PATH_MAX];

	setlocale(LC_ALL, "");
	init_ncurses();

	data.status = newwin(1, COLS, LINES - 1, 0);
	keypad(data.status, TRUE);
	nodelay(data.status, TRUE);

	dirmodel_init(&data.model, getcwd(cwd, PATH_MAX));
	listview_init(&data.view, &data.model, 0, 0, COLS, LINES - 1);

	ev_set_userdata(loop, &data);

	ev_io_init(&stdin_watcher, stdin_cb, 0, EV_READ);
	ev_io_start(loop, &stdin_watcher);

	ev_signal_init(&sigwinch_watcher, sigwinch_cb, SIGWINCH);
	ev_signal_start(loop, &sigwinch_watcher);

	ev_run(loop, 0);

	dirmodel_free(&data.model);
	endwin();

	return 0;
}
