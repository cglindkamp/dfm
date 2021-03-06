/* See LICENSE file for copyright and license details. */
#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <ncurses.h>
#include <stdbool.h>

struct listview {
	WINDOW *window;
	struct listmodel *model;
	size_t first;
	size_t index;
	/*
	 * this variable might never really be needed, if the code calling
	 * listview_refresh also knows, when a refresh is needed, but we keep
	 * it to mark all places in the code, after which a refresh is needed
	 */
	bool needs_refresh;
};

void listview_refresh(struct listview *view);
void listview_up(struct listview *view);
void listview_down(struct listview *view);
void listview_pageup(struct listview *view);
void listview_pagedown(struct listview *view);
void listview_setindex(struct listview *view, size_t index);
size_t listview_getindex(struct listview *view);
size_t listview_getfirst(struct listview *view);
void listview_resize(struct listview *view, unsigned int width, unsigned int height);
bool listview_init(struct listview *view, struct listmodel *model, unsigned int x, unsigned int y, unsigned int width, unsigned int height) __attribute__((warn_unused_result));
void listview_destroy(struct listview *view);

#endif
