#ifndef LIST_H
#define LIST_H

typedef struct list list_t;

list_t *list_new(int initial_size);
void list_free(list_t *list);

int list_length(list_t *list);
void list_append(list_t *list, void *item);
int list_insert(list_t *list, int index, void *item);
void *list_get_item(list_t *list, int index);
void list_sort(list_t *list, int (*compare)(const void *, const void *));

#endif
