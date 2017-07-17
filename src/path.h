/* See LICENSE file for copyright and license details. */
#ifndef PATH_H
#define PATH_H

#include <stdbool.h>
#include <stddef.h>

struct path {
	char *path;
	size_t allocated_size;
};

const char *path_tocstr(struct path *path);
bool path_add_component(struct path *path, const char *component) __attribute__((warn_unused_result));
bool path_remove_component(struct path *path, const char **component);
bool path_set_to_current_working_directory(struct path *path) __attribute__((warn_unused_result));
int path_set_from_string(struct path *path, const char *cstr) __attribute__((warn_unused_result));
int path_new_from_string(struct path **path, const char *cstr) __attribute__((warn_unused_result));
struct path *path_new(void) __attribute__((warn_unused_result));
bool path_init(struct path *path, size_t size) __attribute__((warn_unused_result));
void path_destroy(struct path *path);
void path_delete(struct path *path);

#endif
