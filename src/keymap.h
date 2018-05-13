/* See LICENSE file for copyright and license details. */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdbool.h>
#include <wchar.h>

struct commandexecutor;

struct keyspec {
	bool iskeycode;
	wint_t key;
};

struct keymap_entry {
	struct keyspec keyspec;
	const char *command;
};

struct keymap {
	struct commandexecutor *commandexecutor;
	struct list *entries;
};

int keymap_handlekey(struct keymap *keymap, wint_t key, bool iskeycode);
int keymap_addmapping(struct keymap *keymap, char *keymapstring);
int keymap_setfromstring(struct keymap *keymap, char *keymapstring);
int keymap_setfromfile(struct keymap *keymap, const char *filename);
void keymap_init(struct keymap *keymap, struct commandexecutor *commandexecutor);
void keymap_destroy(struct keymap *keymap);

#endif
