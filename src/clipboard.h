/* See LICENSE file for copyright and license details. */
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "list.h"

struct clipboard {
	const char *path;
	const list_t *filelist;
	const char *shared_clipboard_path;
};

bool clipboard_set_contents(struct clipboard *clipboard, const char *path, const list_t *filelist);
bool clipboard_dump_contents_to_directory(struct clipboard *clipboard, int dir_fd);
void clipboard_init(struct clipboard *clipboard, const char *shared_clipboard_path);
void clipboard_destroy(struct clipboard *clipboard);

#endif
