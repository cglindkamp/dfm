#include <fcntl.h>
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
		if((ret = write(fd, file, strlen(file)) < 0))
			break;
		if((ret = write(fd, "\0", 1)) < 0)
			break;
	}
	close(fd);

	return ret < 0 ? false : true;
}

static bool run_in_foreground()
{
	const char *display = getenv("DISPLAY");

	if(display == NULL || strlen(display) == 0)
		return true;

	const char *foreground = getenv("FILES_FOREGROUND");

	if(foreground != NULL && strcmp(foreground, "yes") == 0)
		return true;

	return false;
}

bool spawn(const char *cwd, const char *program, char * const argv[])
{
	bool foreground = run_in_foreground();

	if(foreground)
		endwin();

	int pid = fork();

	if(pid < 0)
		return false;
	if(pid > 0) {
		if(foreground) {
			waitpid(pid, NULL, 0);
			doupdate();
		}
		return true;
	}

	if(!foreground) {
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
	}
	chdir(cwd);
	execvp(program, argv);
	exit(EXIT_FAILURE);
}
