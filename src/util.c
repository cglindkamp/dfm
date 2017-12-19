/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <ftw.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "path.h"
#include "util.h"
#include "xdg.h"

static int remove_file(const char *path, const struct stat *sbuf, int type, struct FTW *ftwb)
{
	(void)sbuf;
	(void)type;
	(void)ftwb;

	remove(path);
	return 0;
}

void remove_directory_recursively(const char *path)
{
	nftw(path, remove_file, 10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS);
}

struct path *determine_usable_config_file(const char *project, const char *subdir, const char *config, int flags)
{
	struct path *path = NULL;
	list_t *list = xdg_get_config_dirs(true);

	if(list == NULL)
		return NULL;

	for(size_t i = 0; i < list_length(list); i++) {
		struct path *curpath = list_get_item(list, i);
		if(!path_add_component(curpath, project))
			goto err;
		if(subdir) {
			if(!path_add_component(curpath, subdir))
				goto err;
		}
		if(!path_add_component(curpath, config))
			goto err;
		if(path == NULL && access(path_tocstr(curpath), flags) == 0) {
			path = curpath;
			list_set_item(list, i, NULL);
		}
	}
err:
	list_delete(list, (list_item_deallocator)path_delete);
	return path;
}

bool dump_string_to_file(int dir_fd, const char *filename, const char *value)
{
	int fd = openat(dir_fd, filename, O_CREAT|O_WRONLY, 0700);
	if(fd < 0)
		return false;

	int ret = write(fd, value, strlen(value));

	close(fd);

	return ret < 0 ? false : true;
}

bool dump_filelist_to_file(int dir_fd, const char *filename, const list_t *list)
{
	int ret = 0;

	int fd = openat(dir_fd, filename, O_CREAT|O_WRONLY, 0700);
	if(fd < 0)
		return false;

	for(size_t i = 0; i < list_length(list); i++) {
		const char *file = list_get_item(list, i);
		if((ret = write(fd, file, strlen(file))) < 0)
			break;
		if((ret = write(fd, "\0", 1)) < 0)
			break;
	}
	close(fd);

	return ret < 0 ? false : true;
}

bool run_in_foreground()
{
	const char *display = getenv("DISPLAY");

	if(display == NULL || strlen(display) == 0)
		return true;

	const char *foreground = getenv("DFM_FOREGROUND");

	if(foreground != NULL && strcmp(foreground, "yes") == 0)
		return true;

	return false;
}
