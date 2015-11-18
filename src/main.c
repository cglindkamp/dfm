#define _XOPEN_SOURCE_EXTENDED
#include <stdbool.h>
#include <locale.h>
#include <ncurses.h>
#include <wchar.h>

#include "listmodel.h"
#include "listview.h"
#include "testmodel.h"

void init_ncurses()
{
	initscr();
	noecho();
	raw();
	curs_set(0);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
}

int main(void)
{
	struct listview view;
	struct listmodel model;
	WINDOW *status;
	wint_t key;
	int ret;
	bool running = true;

	setlocale(LC_ALL, "");
	init_ncurses();

	status = newwin(1, COLS, LINES - 1, 0);
	keypad(status, TRUE);

	testmodel_init(&model);
	listview_init(&view, &model, 0, 0, COLS, LINES - 1);

	while(running) {
		ret = wget_wch(status, &key);
		if(ret == ERR)
			continue;

		wmove(status, 0, 0);
		wprintw(status, "%-3d %-10d", ret, key);
		wrefresh(status);

		if(ret == KEY_CODE_YES) {
			switch(key) {
			case KEY_RESIZE:
				mvwin(status, LINES - 1, 0);
				wresize(status, 1, COLS);
				listview_resize(&view, COLS, LINES - 1);
				break;
			case KEY_UP:
				listview_up(&view);
				break;
			case KEY_DOWN:
				listview_down(&view);
				break;
			}
		}
		if(ret == OK && key == 3) // ^C
			running = false;
	}

	endwin();

	return 0;
}
