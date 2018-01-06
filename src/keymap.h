/* See LICENSE file for copyright and license details. */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdbool.h>
#include <wchar.h>

struct application;
struct command_map;

struct keyspec {
	bool iskeycode;
	wint_t key;
};

struct keymap {
	struct keyspec keyspec;
	const char *command;
};

int keymap_handlekey(struct keymap *keymap, struct application *application, wint_t key, bool iskeycode, struct command_map *commandmap);
int keymap_newfromstring(struct keymap **keymap, char *keymapstring, struct command_map *commandmap);
int keymap_newfromfile(struct keymap **keymap, const char *filename, struct command_map *commandmap);
void keymap_delete(struct keymap *keymap);

#endif
