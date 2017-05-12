#include <stdlib.h>

#include "listmodel.h"

struct callback {
	model_change_callback *cb;
	void *data;
};

size_t listmodel_count(struct listmodel *model)
{
	return model->count(model);
}

size_t listmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, size_t width, size_t index)
{
	return model->render(model, buffer, len, width, index);
}

void listmodel_setmark(struct listmodel *model, size_t index, bool mark)
{
	model->setmark(model, index, mark);
}

bool listmodel_ismarked(struct listmodel *model, size_t index)
{
	return model->ismarked(model, index);
}

void listmodel_register_change_callback(struct listmodel *model, model_change_callback callback, void *data)
{
	struct callback *cb = malloc(sizeof(*cb));
	cb->cb = callback;
	cb->data = data;
	list_append(model->change_callbacks, cb);
}

void listmodel_unregister_change_callback(struct listmodel *model, model_change_callback callback, void *data)
{
	struct list *list = model->change_callbacks;
	struct callback *cb;
	size_t i;

	for(i = 0; i < list_length(list); i++) {
		cb = list_get_item(list, i);
		if(cb->cb == callback && cb->data == data) {
			free(cb);
			list_remove(list, i);
		}
	}
}

void listmodel_notify_change(struct listmodel *model, size_t index, enum model_change change)
{
	struct list *list = model->change_callbacks;
	struct callback *cb;
	size_t i;

	for(i = 0; i < list_length(list); i++) {
		cb = list_get_item(list, i);
		cb->cb(index, change, cb->data);
	}
}

void listmodel_init(struct listmodel *model)
{
	model->change_callbacks = list_new(0);
}

void listmodel_free(struct listmodel *model)
{
	struct list *list = model->change_callbacks;
	size_t i;

	for(i = 0; i < list_length(list); i++)
		free(list_get_item(list, i));
	list_free(list);
}
