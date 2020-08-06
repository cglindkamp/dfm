/* See LICENSE file for copyright and license details. */
#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stddef.h>

typedef void (*list_item_deallocator)(void *);

struct list *list_new(size_t initial_size) __attribute__((warn_unused_result));
void list_delete(const struct list *list, list_item_deallocator deallocator);

size_t list_length(const struct list *list);
bool list_append(struct list *list, void *item) __attribute__((warn_unused_result));
bool list_insert(struct list *list, size_t index, void *item) __attribute__((warn_unused_result));
void list_move_item(struct list *list, size_t oldindex, size_t newindex);
void list_remove(struct list *list, size_t index);
void *list_get_item(const struct list *list, size_t index);
void list_set_item(struct list *list, size_t index, void *item);
void list_sort(struct list *list, int (*compare)(const void *, const void *));
bool list_find_item_or_insertpoint(const struct list *list, int (*compare)(const void *, const void *), const void *item, size_t *index);

#endif
