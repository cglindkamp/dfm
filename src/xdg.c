/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>

#include "xdg.h"

#include "path.h"

bool xdg_get_config_home(struct path *path)
{
	bool valid = false;

	const char *config_home = getenv("XDG_CONFIG_HOME");
	if(config_home)
		valid = path_set_from_string(path, config_home);

	if(!valid) {
		const char *home = getenv("HOME");
		if(home)
			valid = path_set_from_string(path, home);
		if(valid)
			path_add_component(path, ".config");
	}
	return valid;
}

list_t *xdg_get_config_dirs(bool include_config_home)
{
	struct path *path;
	list_t *list = list_new(0);

	const char *config_dirs = getenv("XDG_CONFIG_DIRS");
	if(config_dirs) {
		char *config_dirs_copy = strdup(config_dirs);
		char *curpath, *saveptr;

		if((curpath = strtok_r(config_dirs_copy, ":", &saveptr))) {
			do {
				path = malloc(sizeof(*path));
				path_init(path, 0);
				if(path_set_from_string(path, curpath))
					list_append(list, path);
				else
					path_free_heap_allocated(path);
			} while((curpath = strtok_r(NULL, ":", &saveptr)));

		}
		free(config_dirs_copy);
	}

	if(list_length(list) == 0) {
		path = malloc(sizeof(*path));
		path_init(path, 0);
		path_set_from_string(path, "/etc/xdg");
		list_append(list, path);
	}

	if(include_config_home) {
		path = malloc(sizeof(*path));
		path_init(path, 0);
		if(xdg_get_config_home(path))
			list_insert(list, 0, path);
		else
			path_free_heap_allocated(path);
	}

	return list;
}

