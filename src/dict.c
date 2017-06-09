/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>

#include "dict.h"

struct dict_item {
	const char *key;
	void *value;
};

list_t *dict_new()
{
	return list_new(0);
}

void dict_free(list_t *list, bool free_value)
{
	struct dict_item *item;
	size_t i;

	for(i = 0; i < list_length(list); i++) {
		item = list_get_item(list, i);
		if(free_value)
			free(item->value);
		free((void*)item->key);
		free(item);
	}
	list_free(list, NULL);
}

static int compare_key(const void *a, const void *b)
{
	struct dict_item *item1 = *(struct dict_item **)a;
	struct dict_item *item2 = *(struct dict_item **)b;

	return strcmp(item1->key, item2->key);
}

void dict_set(list_t *list, const char *key, void *value)
{
	struct dict_item *item = malloc(sizeof(*item));
	size_t index;
	bool found;

	item->key = key;

	found = list_find_item_or_insertpoint(list, compare_key, item, &index);
	if(found) {
		free(item);
		item = list_get_item(list, index);
	} else {
		list_insert(list, index, item);
		item->key = strdup(key);
	}
	item->value = value;
}

void *dict_get(list_t *list, const char *key)
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
