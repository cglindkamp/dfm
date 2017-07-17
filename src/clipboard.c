/* See LICENSE file for copyright and license details. */
#include <stdlib.h>

#include "clipboard.h"

void clipboard_set_contents(struct clipboard *clipboard, const char *path, const list_t *filelist)
{
	clipboard_destroy(clipboard);
	clipboard->path = path;
	clipboard->filelist = filelist;
}

const char *clipboard_get_path(struct clipboard *clipboard)
{
	return clipboard->path;
}

const list_t *clipboard_get_filelist(struct clipboard *clipboard)
{
	return clipboard->filelist;
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
