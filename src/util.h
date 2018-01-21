/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

struct list;

#define container_of(ptr, type, member) \
	((type *)((char *)ptr - offsetof(type, member)))

void remove_directory_recursively(const char *path);
struct path *determine_usable_config_file(const char *project, const char *subdir, const char *config, int flags);
bool dump_string_to_file(int dir_fd, const char *filename, const char *value);
bool dump_filelist_to_file(int dir_fd, const char *filename, const struct list *list);
bool run_in_foreground();

#endif
