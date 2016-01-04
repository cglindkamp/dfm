#include <stdlib.h>

#include "listmodel.h"

struct callback {
	model_change_callback *cb;
	void *data;
};

unsigned int listmodel_count(struct listmodel *model)
{
	return model->count(model);
}

void listmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index)
{
	model->render(model, buffer, len, index);
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
	unsigned int i;

	for(i = 0; i < list_length(list); i++) {
		cb = list_get_item(list, i);
		if(cb->cb == callback && cb->data == data) {
			free(cb);
			list_remove(list, i);
		}
	}
}

void listmodel_notify_change(struct listmodel *model, unsigned int index, enum model_change change)
{
	struct list *list = model->change_callbacks;
	struct callback *cb;
	unsigned int i;

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
	unsigned int i;

	for(i = 0; i < list_length(list); i++)
		free(list_get_item(list, i));
	list_free(list);
}
