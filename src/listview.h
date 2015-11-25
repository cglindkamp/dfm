#ifndef LISTVIEW_H
#define LISTVIEW_H

struct listview {
	WINDOW *window;
	struct listmodel *model;
	unsigned int first;
	unsigned int index;
};

void listview_up(struct listview *view);
void listview_down(struct listview *view);
void listview_pageup(struct listview *view);
void listview_pagedown(struct listview *view);
void listview_resize(struct listview *view, unsigned int width, unsigned int height);
void listview_init(struct listview *view, struct listmodel *model, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

#endif
