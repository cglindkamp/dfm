/* See LICENSE file for copyright and license details. */
#include "listview.h"

#include "listmodel.h"

#include <ncurses.h>
#include <wchar.h>

static int attrs_for_index(struct listmodel *model, size_t index, size_t selected)
{
	if(index == selected) {
		if(listmodel_ismarked(model, index))
			return A_BOLD | COLOR_PAIR(3);
		else
			return A_BOLD | COLOR_PAIR(1);
	} else {
		if(listmodel_ismarked(model, index))
			return COLOR_PAIR(2);
	}
	return 0;
}

void listview_refresh(struct listview *view)
{
	if(!view->needs_refresh)
		return;

	unsigned int height, width;
	size_t i = 0;
	height = getmaxy(view->window);
	width = getmaxx(view->window);

	size_t buflen = width;

	werase(view->window);
	while(i < height) {
		if(i + view->first >= listmodel_count(view->model))
			break;

		wchar_t buffer[buflen + 1];
		size_t reallen = listmodel_render(view->model, buffer, buflen, width, i + view->first);

		if(reallen > buflen) {
			buflen = reallen;
			continue;
		}

		int attrs = attrs_for_index(view->model, view->first + i, view->index);

		wmove(view->window, i, 0);
		wattron(view->window, attrs);
		waddwstr(view->window, buffer);
		wattroff(view->window, attrs);

		i++;
	}
	wrefresh(view->window);
	view->needs_refresh = false;
}

static void listview_setindex_internal(struct listview *view, size_t index, int direction, bool keep_distance)
{
	size_t listcount = listmodel_count(view->model);
	unsigned int rowcount = getmaxy(view->window);
	size_t distance = view->index - view->first;

	/* check index for over- and underflow */
	if(direction == 0 && index > listcount - 1)
		view->index = listcount - 1;
	else if(direction < 0 && index > view->index)
		view->index = 0;
	else if(direction > 0 && (index < view->index || index > listcount - 1))
		view->index = listcount - 1;
	else
		view->index = index;

	/* try to restore distance between first and index
	 * may underflow, but this is checked later */
	if(keep_distance)
		view->first = view->index - distance;

	/* make sure, first has a sane value relative to index */
	if(view->index < view->first)
		view->first = view->index;
	else if(view->index - view->first >= rowcount)
		view->first = view->index - rowcount + 1;

	/* make sure, view is always filled to the bottom with entries,
	 * or in case there aren't enough, scrolled to the first entry */
	if(listcount <= rowcount)
		view->first = 0;
	else if(listcount - view->first <= rowcount)
		view->first = listcount - rowcount;

	view->needs_refresh = true;
}

void listview_up(struct listview *view)
{
	listview_setindex_internal(view, view->index - 1, -1, false);
}

void listview_down(struct listview *view)
{
	listview_setindex_internal(view, view->index + 1, 1, false);
}

void listview_pageup(struct listview *view)
{
	unsigned int rowcount = getmaxy(view->window);

	if(view->index == view->first)
		listview_setindex_internal(view, view->index - rowcount, -1, false);
	else
		listview_setindex_internal(view, view->first, -1, false);
}

void listview_pagedown(struct listview *view)
{
	unsigned int rowcount = getmaxy(view->window);

	if(view->index - view->first < rowcount - 1)
		listview_setindex_internal(view, view->first + rowcount - 1, 1, false);
	else
		listview_setindex_internal(view, view->index + rowcount, 1, false);
}

void listview_setindex(struct listview *view, size_t index)
{
	listview_setindex_internal(view, index, 0, false);
}

size_t listview_getindex(struct listview *view)
{
	return view->index;
}

size_t listview_getfirst(struct listview *view)
{
	return view->first;
}

void listview_resize(struct listview *view, unsigned int width, unsigned int height)
{
	wresize(view->window, height, width);
	listview_setindex_internal(view, view->index, 0, false);
}

static void change_cb(enum model_change change, size_t newindex, size_t oldindex, void *data)
{
	struct listview *view = data;
	unsigned int rowcount = getmaxy(view->window);

	switch(change) {
	case MODEL_ADD:
		if(newindex <= view->index)
			listview_setindex_internal(view, view->index + 1, 1, true);
		else if(newindex < view->first + rowcount)
			view->needs_refresh = true;
		break;
	case MODEL_REMOVE:
		if(oldindex < view->index)
			listview_setindex_internal(view, view->index - 1, -1, true);
		else if(oldindex < view->first + rowcount)
			listview_setindex_internal(view, view->index, 0, true);
		break;
	case MODEL_CHANGE:
		if(newindex == oldindex) {
			if(newindex >= view->first && newindex < view->first + rowcount)
				view->needs_refresh = true;
		} else if(oldindex == view->index) {
			listview_setindex_internal(view, newindex, 0, false);
		} else {
			change_cb(MODEL_ADD, newindex, oldindex, data);
			change_cb(MODEL_REMOVE, newindex, oldindex, data);
		}
		break;
	case MODEL_RELOAD:
		view->first = 0;
		view->index = 0;
		view->needs_refresh = true;
	}
}

bool listview_init(struct listview *view, struct listmodel *model, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	view->window = newwin(height, width, y, x);
	if(view->window == NULL)
		return false;

	view->index = 0;
	view->first = 0;
	view->model = model;
	view->needs_refresh = true;

	if(!listmodel_register_change_callback(view->model, change_cb, view)) {
		delwin(view->window);
		view->window = NULL;
		return false;
	}

	listview_refresh(view);

	return true;
}

void listview_destroy(struct listview *view)
{
	if(view->window != NULL) {
		listmodel_unregister_change_callback(view->model, change_cb, view);
		delwin(view->window);
	}
}

