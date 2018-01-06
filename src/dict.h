/* See LICENSE file for copyright and license details. */
#ifndef DICT_H
#define DICT_H

#include <stdbool.h>

struct list;

struct list *dict_new() __attribute__((warn_unused_result));
void dict_delete(struct list *list, bool free_value);

bool dict_set(struct list *list, const char *key, void *value) __attribute__((warn_unused_result));
void *dict_get(struct list *list, const char *key);

#endif
