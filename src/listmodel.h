#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <wchar.h>

struct listmodel {
	unsigned int (*count)(struct listmodel *model);
	void (*render)(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index);
};

unsigned int listmodel_count(struct listmodel *model);
void listmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index);

#endif
