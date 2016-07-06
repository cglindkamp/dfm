#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <wchar.h>

#include "listmodel.h"
#include "listview.h"

static void print_list(struct listview *view)
{
	unsigned int height, width;
	size_t i;
	height = getmaxy(view->window);
	width = getmaxx(view->window);

	wchar_t buffer[width+1];

	werase(view->window);
	for(i = 0; i < height; i++) {
		if(i + view->first >= listmodel_count(view->model))
			break;

		if(i + view->first == view->index) {
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
	if(view->index > 0) {
		view->index--;
		if(view->first > view->index)
			view->first = view->index;
	}
	print_list(view);
}

void listview_down(struct listview *view)
{
	unsigned int height = getmaxy(view->window);

	if(view->index < listmodel_count(view->model) - 1) {
		view->index++;
		if(view->index - view->first > height - 1)
			view->first++;
	}
	print_list(view);
}

void listview_pageup(struct listview *view)
{
	unsigned int rowcount = getmaxy(view->window);

	if(view->index == view->first) {
		if(view->first >= rowcount)
			view->first -= rowcount;
		else
			view->first = 0;
	}
	view->index = view->first;
	print_list(view);
}

void listview_pagedown(struct listview *view)
{
	size_t listcount = listmodel_count(view->model);
	unsigned int rowcount = getmaxy(view->window);

	if(view->index - view->first < rowcount - 1) {
		if(listcount > rowcount)
			view->index = view->first + rowcount - 1;
		else
			view->index = listcount - 1;
	} else {
		if(view->index + rowcount < listcount)
			view->index += rowcount;
		else {
			view->index = listcount - 1;
		}
		view->first = view->index - rowcount + 1;
	}
	print_list(view);
}

size_t listview_getindex(struct listview *view)
{
	return view->index;
}

void listview_resize(struct listview *view, unsigned int width, unsigned int height)
{
	size_t count = listmodel_count(view->model);

	wresize(view->window, height, width);
	if(view->index - view->first >= height)
		view->first = view->index - height + 1;
	if(view->first + height > count) {
		if(height > count)
			view->first = 0;
		else
			view->first = count - height;
	}
	print_list(view);
}

static void change_cb(unsigned int index, enum model_change change, void *data)
{
	struct listview *view = data;
	unsigned int rowcount = getmaxy(view->window);
	size_t listcount = listmodel_count(view->model);

	switch(change) {
	case MODEL_ADD:
		if(listcount == 1) {
			view->index = 0;
			print_list(view);
		} else if(index <= view->index) {
			view->index++;
			if(listcount > rowcount)
				view->first++;
			print_list(view);
		} else if(index < view->first + rowcount)
			print_list(view);
		break;
	case MODEL_REMOVE:
		if(index <= view->index) {
			view->index--;
			if(view->first != 0)
				view->first--;
			print_list(view);
		} else if(index < view->first + rowcount)
			print_list(view);
	case MODEL_CHANGE:
		if(index >= view->first && index < view->first + rowcount)
			print_list(view);
		break;
	case MODEL_RELOAD:
		view->first = 0;
		view->index = 0;
		print_list(view);
	}
}

void listview_init(struct listview *view, struct listmodel *model, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	view->window = newwin(height, width, y, x);
	view->index = 0;
	view->first = 0;
	view->model = model;
	listmodel_register_change_callback(view->model, change_cb, view);
	print_list(view);
}

void listview_free(struct listview *view)
{
	listmodel_unregister_change_callback(view->model, change_cb, view);
	delwin(view->window);
}

