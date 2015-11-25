#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <wchar.h>

#include "listmodel.h"
#include "listview.h"

static void print_list(struct listview *view)
{
	int i,height, width;
	height = getmaxy(view->window);
	width = getmaxx(view->window);

	wchar_t buffer[width+1];

	werase(view->window);
	for(i = 0; i < height; i++) {
		if(i + view->first >= listmodel_count(view->model))
			break;

		if(i == view->index) {
			wattron(view->window, A_BOLD | COLOR_PAIR(1) );
		} else {
			wattroff(view->window, A_BOLD | COLOR_PAIR(1) );
		}
		wmove(view->window, i, 0);
		listmodel_render(view->model, buffer, width, i + view->first);
		waddwstr(view->window, buffer);
	}
	wrefresh(view->window);
}

void listview_up(struct listview *view)
{
	if(view->index > 0)
		view->index--;
	else if(view->first > 0)
		view->first--;
	print_list(view);
}

void listview_down(struct listview *view)
{
	if(view->first + view->index < listmodel_count(view->model) - 1) {
		if(view->index < getmaxy(view->window) - 1)
			view->index++;
		else
			view->first++;
	}
	print_list(view);
}

void listview_pageup(struct listview *view)
{
	int rowcount = getmaxy(view->window);

	if(view->index > 0)
		view->index = 0;
	else {
		if(view->first >= rowcount)
			view->first -= rowcount;
		else
			view->first = 0;
	}
	print_list(view);
}

void listview_pagedown(struct listview *view)
{
	int listcount = listmodel_count(view->model);
	int rowcount = getmaxy(view->window);

	if(view->index < rowcount - 1) {
		if(listcount > rowcount)
			view->index = rowcount - 1;
		else
			view->index = listcount - 1;
	} else {
		if(view->first + view->index + rowcount < listcount)
			view->first += rowcount;
		else {
			view->first = listcount - rowcount;
		}
	}
	print_list(view);
}

void listview_resize(struct listview *view, unsigned int width, unsigned int height)
{
	unsigned int selected = view->first + view->index;
	unsigned int count = listmodel_count(view->model);

	wresize(view->window, height, width);
	if(view->index >= height) {
		view->index = height - 1;
		view->first = selected - view->index;
	}
	if(view->first + height > count) {
		if(height > count)
			view->first = 0;
		else
			view->first = count - height;
		view->index = selected - view->first;
	}
	print_list(view);
}

void listview_init(struct listview *view, struct listmodel *model, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	view->window = newwin(height, width, y, x);
	view->index = 0;
	view->first = 0;
	view->model = model;
	print_list(view);
}

