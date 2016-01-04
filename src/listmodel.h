#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <wchar.h>

#include "list.h"

struct listmodel {
	unsigned int (*count)(struct listmodel *model);
	void (*render)(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index);
	list_t *change_callbacks;
	void *data;
};

enum model_change {
	MODEL_ADD,
	MODEL_REMOVE,
	MODEL_CHANGE,
	MODEL_RELOAD,
};

typedef void(model_change_callback)(unsigned int index, enum model_change change, void *data);

unsigned int listmodel_count(struct listmodel *model);
void listmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index);
void listmodel_register_change_callback(struct listmodel *model, model_change_callback callback, void *data);
void listmodel_unregister_change_callback(struct listmodel *model, model_change_callback callback, void *data);

#endif
