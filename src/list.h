/* See LICENSE file for copyright and license details. */
#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct list list_t;
typedef void (*list_item_deallocator)(void *);

list_t *list_new(size_t initial_size);
void list_free(const list_t *list, list_item_deallocator deallocator);

size_t list_length(const list_t *list);
void list_append(list_t *list, void *item);
void list_insert(list_t *list, size_t index, void *item);
void list_remove(list_t *list, size_t index);
void *list_get_item(const list_t *list, size_t index);
void list_set_item(list_t *list, size_t index, void *item);
void list_sort(list_t *list, int (*compare)(const void *, const void *));
bool list_find_item_or_insertpoint(const list_t *list, int (*compare)(const void *, const void *), void *item, size_t *index);

#endif
