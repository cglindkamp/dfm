/* See LICENSE file for copyright and license details. */
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "list.h"

struct clipboard {
	const char *path;
	const list_t *filelist;
};

void clipboard_set_contents(struct clipboard *clipboard, const char *path, const list_t *filelist);
const char *clipboard_get_path(struct clipboard *clipboard);
const list_t *clipboard_get_filelist(struct clipboard *clipboard);
void clipboard_init(struct clipboard *clipboard);
void clipboard_free(struct clipboard *clipboard);

#endif
