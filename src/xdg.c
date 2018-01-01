/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "xdg.h"

#include "path.h"

int xdg_get_config_home(struct path **path)
{
	int ret = 0;
	*path = NULL;

	const char *config_home = getenv("XDG_CONFIG_HOME");
	if(config_home)
		ret = path_new_from_string(path, config_home);

	if(ret == ENOMEM)
		return ENOMEM;

	if(*path == NULL) {
		const char *home = getenv("HOME");
		if(home) {
			ret = path_new_from_string(path, home);

			if(ret == ENOMEM)
				return ENOMEM;

			if(*path == NULL)
				return ENOENT;

			if(!path_add_component(*path, ".config")) {
				path_delete(*path);
				*path = NULL;
				return ENOMEM;
			}
		} else
			return ENOENT;
	}
	return 0;
}

struct list *xdg_get_config_dirs(bool include_config_home)
{
	struct path *path;
	int ret;

	struct list *list = list_new(0);
	if(list == NULL)
		return NULL;

	const char *config_dirs = getenv("XDG_CONFIG_DIRS");
	if(config_dirs) {
		char *curpath, *saveptr;
		char config_dirs_copy[strlen(config_dirs) + 1];

		strcpy(config_dirs_copy, config_dirs);

		if((curpath = strtok_r(config_dirs_copy, ":", &saveptr))) {
			do {
				ret = path_new_from_string(&path, curpath);
				if(ret == EINVAL)
					continue;
				if(path != NULL) {
					if(!list_append(list, path))
						goto err_delete_list_and_path;
				} else
					goto err_delete_list;
			} while((curpath = strtok_r(NULL, ":", &saveptr)));

		}
	}

	if(list_length(list) == 0) {
		ret = path_new_from_string(&path, "/etc/xdg");
		if(path != NULL) {
			if(!list_append(list, path))
				goto err_delete_list_and_path;
		} else
			goto err_delete_list;
	}

	if(include_config_home) {
		ret = xdg_get_config_home(&path);
		if(path != NULL) {
			if(!list_insert(list, 0, path))
				goto err_delete_list_and_path;
		} else if(ret == ENOMEM)
			goto err_delete_list;
	}

	return list;

err_delete_list_and_path:
	path_delete(path);
err_delete_list:
	list_delete(list, (list_item_deallocator)path_delete);
	return NULL;
}

