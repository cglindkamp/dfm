/* See LICENSE file for copyright and license details. */
#ifndef DICT_H
#define DICT_H

#include "list.h"

list_t *dict_new() __attribute__((warn_unused_result));
void dict_delete(list_t *list, bool free_value);

bool dict_set(list_t *list, const char *key, void *value) __attribute__((warn_unused_result));
void *dict_get(list_t *list, const char *key);

#endif
