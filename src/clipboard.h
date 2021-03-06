/* See LICENSE file for copyright and license details. */
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <stdbool.h>

struct list;

struct clipboard {
	const char *path;
	const struct list *filelist;
	const char *shared_clipboard_path;
};

bool clipboard_set_contents(struct clipboard *clipboard, const char *path, const struct list *filelist);
bool clipboard_dump_contents_to_directory(struct clipboard *clipboard, int dir_fd);
void clipboard_init(struct clipboard *clipboard, const char *shared_clipboard_path);
void clipboard_destroy(struct clipboard *clipboard);

#endif
