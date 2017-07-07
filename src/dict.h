/* See LICENSE file for copyright and license details. */
#ifndef DICT_H
#define DICT_H

#include "list.h"

list_t *dict_new();
void dict_free(list_t *list, bool free_value);

bool dict_set(list_t *list, const char *key, void *value);
void *dict_get(list_t *list, const char *key);

#endif
