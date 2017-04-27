#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "path.h"

static void path_make_room(struct path *path)
{
	path->allocated_size *= 2;
	path->path = realloc(path->path, path->allocated_size);
}

const char *path_tocstr(struct path *path)
{
	if(path->path[0] == '\0')
		return "/";
	return path->path;
}

void path_add_component(struct path *path, const char *component)
{
	while(strlen(path->path) + strlen(component) + 2 > path->allocated_size)
		path_make_room(path);
	strcat(path->path, "/");
	strcat(path->path, component);
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
		if(errno == ERANGE)
			path_make_room(path);
		else
			return false;
	}
	return true;
}

void path_init(struct path *path, size_t size)
{
	path->allocated_size = size;
	if(path->allocated_size < 1)
		path->allocated_size = 1;
	path->path = malloc(path->allocated_size);
	path->path[0] = '\0';
}

void path_free(struct path *path)
{
	free(path->path);
}
