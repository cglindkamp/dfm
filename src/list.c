#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

struct list {
	int length;
	int allocated_items;
	void **items;
};

list_t *list_new(int initial_size)
{
	list_t *list;

	list = calloc(1, sizeof(list_t));
	list->allocated_items = initial_size;
	if(list->allocated_items <= 0)
		list->allocated_items = 8;
	list->items = calloc(list->allocated_items, sizeof(void *));

	return list;
}

void list_free(list_t *list)
{
	free(list->items);
	free(list);
}

int list_length(list_t *list)
{
	return list->length;
}

static void list_make_room(list_t *list)
{
	list->allocated_items *= 2;
	list->items = realloc(list->items, list->allocated_items * sizeof(void *));
}

void list_append(list_t *list, void *item)
{
	if(list->length == list->allocated_items)
		list_make_room(list);

	list->items[list->length] = item;
	list->length++;
}

void list_insert(list_t *list, int index, void *item)
{
	assert(index >= 0 && index <= list->length);

	if(list->length == list->allocated_items)
		list_make_room(list);

	memmove(&list->items[index + 1], &list->items[index], (list->length - index) * sizeof(list->items[0]));
	list->items[index] = item;
	list->length++;
}

void *list_get_item(list_t *list, int index)
{
	assert(index >= 0 && index < list->length);

	return list->items[index];
}

void list_sort(list_t *list, int (*compare)(const void *, const void *))
{
	qsort(list->items, list->length, sizeof(void *), compare);
}
