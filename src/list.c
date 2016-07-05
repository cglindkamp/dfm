#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

struct list {
	size_t length;
	size_t allocated_items;
	void **items;
};

list_t *list_new(size_t initial_size)
{
	list_t *list;

	list = calloc(1, sizeof(list_t));
	list->allocated_items = initial_size;
	if(list->allocated_items == 0)
		list->allocated_items = 8;
	list->items = calloc(list->allocated_items, sizeof(void *));

	return list;
}

void list_free(list_t *list)
{
	free(list->items);
	free(list);
}

size_t list_length(list_t *list)
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

void list_insert(list_t *list, size_t index, void *item)
{
	assert(index <= list->length);

	if(list->length == list->allocated_items)
		list_make_room(list);

	memmove(&list->items[index + 1], &list->items[index], (list->length - index) * sizeof(list->items[0]));
	list->items[index] = item;
	list->length++;
}

void list_remove(list_t *list, size_t index)
{
	assert(index < list->length);

	memmove(&list->items[index], &list->items[index + 1], (list->length - index - 1) * sizeof(list->items[0]));
	list->length--;
}

void *list_get_item(list_t *list, size_t index)
{
	assert(index < list->length);

	return list->items[index];
}

void list_set_item(list_t *list, size_t index, void *item)
{
	assert(index < list->length);

	list->items[index] = item;
}

void list_sort(list_t *list, int (*compare)(const void *, const void *))
{
	qsort(list->items, list->length, sizeof(void *), compare);
}

bool list_find_item_or_insertpoint(list_t *list, int (*compare)(const void *, const void *), void *item, size_t *index)
{
	void *currentitem;
	size_t min, middle, max;
	int ret;

	if(list_length(list) == 0) {
		*index = 0;
		return false;
	}

	min = 0;
	max = list_length(list) - 1;

	while(min < max) {
		middle = (min + max) / 2;
		currentitem = list_get_item(list, middle);
		ret = compare(&item, &currentitem);
		if(ret <= 0)
			max = middle;
		else
			min = middle + 1;
	}

	*index = min;
	currentitem = list_get_item(list, *index);
	ret = compare(&item, &currentitem);

	if(ret > 0 && *index == list_length(list) - 1)
		*index = list_length(list);
	return ret == 0;

}

