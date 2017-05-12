#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

struct path {
	char *path;
	size_t allocated_size;
};

const char *path_tocstr(struct path *path);
void path_add_component(struct path *path, const char *component);
bool path_remove_component(struct path *path, const char **component);
bool path_set_to_current_working_directory(struct path *path);
void path_init(struct path *path, size_t size);
void path_free(struct path *path);

#endif