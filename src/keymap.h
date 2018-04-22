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

struct keymap_entry {
	struct keyspec keyspec;
	const char *command;
};

struct keymap {
	struct keymap_entry *entries;
};

int keymap_handlekey(struct keymap *keymap, struct application *application, wint_t key, bool iskeycode, struct command_map *commandmap);
int keymap_setfromstring(struct keymap *keymap, char *keymapstring, struct command_map *commandmap);
int keymap_setfromfile(struct keymap *keymap, const char *filename, struct command_map *commandmap);
void keymap_init(struct keymap *keymap);
void keymap_destroy(struct keymap *keymap);

#endif
