/* See LICENSE file for copyright and license details. */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdbool.h>
#include <wchar.h>

struct application;

typedef void (*command_ptr)(struct application *, const char *);
typedef command_ptr (*command_map_ptr)(const char *);

struct command_map {
	const char *command_name;
	command_ptr command;
	bool param_mandatory;
};

struct keyspec {
	bool iskeycode;
	wint_t key;
};

struct keymap {
	struct keyspec keyspec;
	command_ptr command;
	const char *param;
};

void keymap_handlekey(struct keymap *keymap, struct application *application, wint_t key, bool iskeycode);
int keymap_newfromstring(struct keymap **keymap, char *keymapstring, struct command_map *commandmap);
int keymap_newfromfile(struct keymap **keymap, const char *filename, struct command_map *commandmap);
void keymap_delete(struct keymap *keymap);

#endif
