/* See LICENSE file for copyright and license details. */
#include "dict.h"

#include "list.h"

#include <stdlib.h>
#include <string.h>

struct dict_item {
	const char *key;
	void *value;
};

struct list *dict_new()
{
	return list_new(0);
}

void dict_delete(struct list *list, bool free_value)
{
	struct dict_item *item;

	if(list == NULL)
		return;

	size_t length = list_length(list);;
	for(size_t i = 0; i < length; i++) {
		item = list_get_item(list, i);
		if(free_value)
			free(item->value);
		free((void*)item->key);
		free(item);
	}
	list_delete(list, NULL);
}

static int compare_key(const void *a, const void *b)
{
	struct dict_item *item1 = *(struct dict_item **)a;
	struct dict_item *item2 = *(struct dict_item **)b;

	return strcmp(item1->key, item2->key);
}

bool dict_set(struct list *list, const char *key, void *value)
{
	struct dict_item *item, needle;

	size_t index;
	bool found;

	needle.key = key;

	found = list_find_item_or_insertpoint(list, compare_key, &needle, &index);
	if(found) {
		item = list_get_item(list, index);
	} else {
		item = malloc(sizeof(*item));
		if(item == NULL)
			return false;

		item->key = strdup(key);
		if(item->key == NULL) {
			free(item);
			return false;
		}
		if(!list_insert(list, index, item)) {
			free((void*)item->key);
			free(item);
			return false;
		}
	}
	item->value = value;

	return true;
}

void *dict_get(struct list *list, const char *key)
{
	struct dict_item item;
	size_t index;
	bool found;

	item.key = key;

	found = list_find_item_or_insertpoint(list, compare_key, &item, &index);
	if(found) {
		struct dict_item *item = list_get_item(list, index);
		return item->value;
	}
	return NULL;
}
