#ifndef DICT_H
#define DICT_H

#include "list.h"

list_t *dict_new();
void dict_free(list_t *list, bool free_value);

void dict_set(list_t *list, char *key, void *value);
void *dict_get(list_t *list, char *key);

#endif
