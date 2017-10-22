/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

#include "list.h"
struct path;

void remove_directory_recursively(const char *path);
struct path *determine_usable_config_file(const char *project, const char *subdir, const char *config, int flags);
bool dump_string_to_file(int dir_fd, const char *filename, const char *value);
bool dump_filelist_to_file(int dir_fd, const char *filename, const list_t *list);
bool run_in_foreground();

#endif
