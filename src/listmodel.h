/* See LICENSE file for copyright and license details. */
#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <wchar.h>

#include "list.h"

struct listmodel {
	size_t (*count)(struct listmodel *model);
	size_t (*render)(struct listmodel *model, wchar_t *buffer, size_t len, size_t width, size_t index);
	void (*setmark)(struct listmodel *model, size_t index, bool mark);
	bool (*ismarked)(struct listmodel *model, size_t index);
	list_t *change_callbacks;
	void *data;
};

enum model_change {
	MODEL_ADD,
	MODEL_REMOVE,
	MODEL_CHANGE,
	MODEL_RELOAD,
};

typedef void(model_change_callback)(size_t index, enum model_change change, void *data);

size_t listmodel_count(struct listmodel *model);
size_t listmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, size_t width, size_t index);
void listmodel_setmark(struct listmodel *model, size_t index, bool mark);
bool listmodel_ismarked(struct listmodel *model, size_t index);
bool listmodel_register_change_callback(struct listmodel *model, model_change_callback callback, void *data);
void listmodel_unregister_change_callback(struct listmodel *model, model_change_callback callback, void *data);

#endif
