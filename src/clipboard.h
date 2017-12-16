/* See LICENSE file for copyright and license details. */
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "list.h"

struct clipboard {
	const char *path;
	const list_t *filelist;
};

void clipboard_set_contents(struct clipboard *clipboard, const char *path, const list_t *filelist);
bool clipboard_dump_contents_to_directory(struct clipboard *clipboard, int dir_fd);
void clipboard_init(struct clipboard *clipboard);
void clipboard_destroy(struct clipboard *clipboard);

#endif
