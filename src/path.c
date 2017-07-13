/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "path.h"

static bool path_make_room(struct path *path, size_t size)
{
	size_t target_size = size ? size : path->allocated_size * 2;

	if(target_size > path->allocated_size) {
		char *newpath = realloc(path->path, target_size);
		if(newpath == NULL)
			return false;

		path->allocated_size = target_size;
		path->path = newpath;
	}
	return true;
}

const char *path_tocstr(struct path *path)
{
	if(path->path[0] == '\0')
		return "/";
	return path->path;
}

bool path_add_component(struct path *path, const char *component)
{
	while(strlen(path->path) + strlen(component) + 2 > path->allocated_size)
		if(!path_make_room(path, 0))
			return false;

	strcat(path->path, "/");
	strcat(path->path, component);

	return true;
}

bool path_remove_component(struct path *path, const char **component)
{
	if(path->path[0] == '\0')
		return false;

	char *slash = strrchr(path->path, '/');
	*slash = '\0';

	if(component)
		*component = slash + 1;

	return true;
}

bool path_set_to_current_working_directory(struct path *path)
{
	while(getcwd(path->path, path->allocated_size) == NULL)
	{
		if(errno == ERANGE) {
			if(!path_make_room(path, 0))
				return false;
		} else
			return false;
	}
	return true;
}

bool path_set_from_string(struct path *path, const char *cstr)
{
	/* paths not starting with '/' are invalid */
	if(cstr[0] != '/')
		return false;

	if(!path_make_room(path, strlen(cstr) + 1))
		return false;
	strcpy(path->path, cstr);

	/* reduce multiple consecutive slashes to a single one */
	size_t s = 0, d = 0;
	do {
		s++;
		if(path->path[s] == '/' && path->path[d] == '/')
			continue;
		d++;
		path->path[d] = path->path[s];
	} while(path->path[s] != '\0');

	/* remove trailing slash, this also ensures that the string will be
	 * empty for the root directory */
	if(path->path[d - 1] == '/')
		path->path[d - 1] = '\0';

	return true;
}

bool path_init(struct path *path, size_t size)
{
	path->allocated_size = size;
	if(path->allocated_size < 1)
		path->allocated_size = 1;

	path->path = malloc(path->allocated_size);
	if(path->path == NULL)
		return false;

	path->path[0] = '\0';

	return true;
}

void path_free(struct path *path)
{
	free(path->path);
}

void path_free_heap_allocated(struct path *path)
{
	path_free(path);
	free(path);
}
