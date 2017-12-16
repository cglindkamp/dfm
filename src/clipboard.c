/* See LICENSE file for copyright and license details. */
#include <stdlib.h>

#include "clipboard.h"
#include "util.h"

void clipboard_set_contents(struct clipboard *clipboard, const char *path, const list_t *filelist)
{
	clipboard_destroy(clipboard);
	clipboard->path = path;
	clipboard->filelist = filelist;
}

bool clipboard_dump_contents_to_directory(struct clipboard *clipboard, int dir_fd)
{
	if(clipboard->path && clipboard->filelist) {
		if(!dump_string_to_file(dir_fd, "clipboard_path", clipboard->path))
			return false;

		if(!dump_filelist_to_file(dir_fd, "clipboard_list", clipboard->filelist))
			return false;
	}

	return true;
}

void clipboard_init(struct clipboard *clipboard)
{
	clipboard->path = NULL;
	clipboard->filelist = NULL;
}

void clipboard_destroy(struct clipboard *clipboard)
{
	free((void *)clipboard->path);
	list_delete(clipboard->filelist, free);
}
