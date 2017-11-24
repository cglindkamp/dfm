/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <ncurses.h>

#include "application.h"

void _nc_freeall();

void init_ncurses()
{
	initscr();
	noecho();
	raw();

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLUE);
}

int main(void)
{
	struct application app;
	int ret = 0;

	setlocale(LC_ALL, "");
	init_ncurses();

	if(application_init(&app)) {
		application_run(&app);
	} else {
		endwin();
		puts("Failed to initialize the application, exiting");
	}
	application_destroy(&app);

	endwin();
	_nc_freeall();

	return ret;
}
