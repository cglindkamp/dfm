#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

typedef struct list list_t;

list_t *list_new(int initial_size);
void list_free(list_t *list);

int list_length(list_t *list);
void list_append(list_t *list, void *item);
void list_insert(list_t *list, int index, void *item);
void list_remove(list_t *list, int index);
void *list_get_item(list_t *list, int index);
void list_set_item(list_t *list, int index, void *item);
void list_sort(list_t *list, int (*compare)(const void *, const void *));
bool list_find_item_or_insertpoint(list_t *list, int (*compare)(const void *, const void *), void *item, unsigned int *index);

#endif
