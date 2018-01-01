/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clipboard.h"
#include "util.h"

#define CLIPBOARD_PATH "clipboard_path"
#define CLIPBOARD_LIST "clipboard_list"

static bool dump_contents_to_directory(int dir_fd, const char *path, const struct list *filelist) {
	if(path && filelist) {
		if(!dump_string_to_file(dir_fd, CLIPBOARD_PATH, path))
			return false;

		if(!dump_filelist_to_file(dir_fd, CLIPBOARD_LIST, filelist))
			return false;
	}
	return true;
}

bool clipboard_set_contents(struct clipboard *clipboard, const char *path, const struct list *filelist)
{
	if(clipboard->shared_clipboard_path) {
		int clipboard_dir_fd = open(clipboard->shared_clipboard_path, O_RDONLY);
		if(clipboard_dir_fd == -1) {
			free((void *)path);
			list_delete(filelist, free);
			return false;
		}

		unlinkat(clipboard_dir_fd, CLIPBOARD_PATH, 0);
		unlinkat(clipboard_dir_fd, CLIPBOARD_LIST, 0);
		bool ret = dump_contents_to_directory(clipboard_dir_fd, path, filelist);

		free((void *)path);
		list_delete(filelist, free);
		close(clipboard_dir_fd);

		return ret;
	} else {
		clipboard_destroy(clipboard);
		clipboard->path = path;
		clipboard->filelist = filelist;
	}

	return true;
}

bool clipboard_dump_contents_to_directory(struct clipboard *clipboard, int dir_fd)
{
	if(clipboard->shared_clipboard_path) {
		int clipboard_dir_fd = open(clipboard->shared_clipboard_path, O_RDONLY);
		if(clipboard_dir_fd == -1)
			return false;

		if((faccessat(clipboard_dir_fd, CLIPBOARD_PATH, F_OK, 0) != 0 &&
		    faccessat(clipboard_dir_fd, CLIPBOARD_LIST, F_OK, 0) != 0) ||
		   (linkat(clipboard_dir_fd, CLIPBOARD_PATH, dir_fd, CLIPBOARD_PATH, 0) == 0 &&
		    linkat(clipboard_dir_fd, CLIPBOARD_LIST, dir_fd, CLIPBOARD_LIST, 0) == 0)) {
			close(clipboard_dir_fd);
			return true;
		}
		close(clipboard_dir_fd);
		return false;
	} else {
		return dump_contents_to_directory(dir_fd, clipboard->path, clipboard->filelist);
	}
}

void clipboard_init(struct clipboard *clipboard, const char *shared_clipboard_path)
{
	clipboard->path = NULL;
	clipboard->filelist = NULL;
	clipboard->shared_clipboard_path = shared_clipboard_path;
}

void clipboard_destroy(struct clipboard *clipboard)
{
	free((void *)clipboard->path);
	list_delete(clipboard->filelist, free);
}
