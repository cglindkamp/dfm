/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

struct list {
	size_t length;
	size_t allocated_items;
	void **items;
};

struct list *list_new(size_t initial_size)
{
	struct list *list;

	list = malloc(sizeof(*list));
	if(list == NULL)
		return NULL;

	list->length = 0;
	list->allocated_items = initial_size;
	if(list->allocated_items == 0)
		list->allocated_items = 1;

	list->items = malloc(list->allocated_items * sizeof(void *));
	if(list->items == NULL) {
		free(list);
		return NULL;
	}

	return list;
}

void list_delete(const struct list *list, list_item_deallocator deallocator)
{
	if(list == NULL)
		return;
	if(deallocator)
		for(size_t i = 0; i < list->length; i++)
			deallocator(list->items[i]);
	free(list->items);
	free((void *)list);
}

size_t list_length(const struct list *list)
{
	return list->length;
}

static bool list_make_room(struct list *list)
{
	void *newitems = realloc(list->items, 2 * list->allocated_items * sizeof(void *));
	if(newitems == NULL)
		return false;

	list->allocated_items *= 2;
	list->items = newitems;

	return true;
}

bool list_append(struct list *list, void *item)
{
	if(list->length == list->allocated_items)
		if(!list_make_room(list))
			return false;

	list->items[list->length] = item;
	list->length++;

	return true;
}

bool list_insert(struct list *list, size_t index, void *item)
{
	assert(index <= list->length);

	if(list->length == list->allocated_items)
		if(!list_make_room(list))
			return false;

	memmove(&list->items[index + 1], &list->items[index], (list->length - index) * sizeof(list->items[0]));
	list->items[index] = item;
	list->length++;

	return true;
}

void list_remove(struct list *list, size_t index)
{
	assert(index < list->length);

	memmove(&list->items[index], &list->items[index + 1], (list->length - index - 1) * sizeof(list->items[0]));
	list->length--;
}

void *list_get_item(const struct list *list, size_t index)
{
	assert(index < list->length);

	return list->items[index];
}

void list_set_item(struct list *list, size_t index, void *item)
{
	assert(index < list->length);

	list->items[index] = item;
}

void list_sort(struct list *list, int (*compare)(const void *, const void *))
{
	qsort(list->items, list->length, sizeof(void *), compare);
}

bool list_find_item_or_insertpoint(const struct list *list, int (*compare)(const void *, const void *), void *item, size_t *index)
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

