#include "keymap.h"

#include "commandexecutor.h"
#include "list.h"

#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int keymap_handlekey(struct keymap *keymap, wint_t key, bool iskeycode)
{
	for(size_t i = 0; i < list_length(keymap->entries); i++) {
		struct keymap_entry *curitem = list_get_item(keymap->entries, i);
		if(curitem->keyspec.key == key && curitem->keyspec.iskeycode == iskeycode) {
			char buffer[strlen(curitem->command) + 1];
			strcpy(buffer, curitem->command);

			return commandexecutor_execute(keymap->commandexecutor, buffer);
		}
	}
	return 0;
}

static struct {
	const char *string;
	struct keyspec keyspec;
} keycodetable[] = {
	{ .string = "space", .keyspec = { .iskeycode = false, .key = L' ' } },
	{ .string = "up", .keyspec = { .iskeycode = true, .key = KEY_UP } },
	{ .string = "down", .keyspec = { .iskeycode = true, .key = KEY_DOWN } },
	{ .string = "left", .keyspec = { .iskeycode = true, .key = KEY_LEFT } },
	{ .string = "right", .keyspec = { .iskeycode = true, .key = KEY_RIGHT } },
	{ .string = "pageup", .keyspec = { .iskeycode = true, .key = KEY_PPAGE } },
	{ .string = "pagedown", .keyspec = { .iskeycode = true, .key = KEY_NPAGE } },
	{ .string = "home", .keyspec = { .iskeycode = true, .key = KEY_HOME } },
	{ .string = "end", .keyspec = { .iskeycode = true, .key = KEY_END } },
	{ .string = "f1", .keyspec = { .iskeycode = true, .key = KEY_F(1) } },
	{ .string = "f2", .keyspec = { .iskeycode = true, .key = KEY_F(2) } },
	{ .string = "f3", .keyspec = { .iskeycode = true, .key = KEY_F(3) } },
	{ .string = "f4", .keyspec = { .iskeycode = true, .key = KEY_F(4) } },
	{ .string = "f5", .keyspec = { .iskeycode = true, .key = KEY_F(5) } },
	{ .string = "f6", .keyspec = { .iskeycode = true, .key = KEY_F(6) } },
	{ .string = "f7", .keyspec = { .iskeycode = true, .key = KEY_F(7) } },
	{ .string = "f8", .keyspec = { .iskeycode = true, .key = KEY_F(8) } },
	{ .string = "f9", .keyspec = { .iskeycode = true, .key = KEY_F(9) } },
	{ .string = "f10", .keyspec = { .iskeycode = true, .key = KEY_F(10) } },
};

static void keymap_string_to_keyspec(struct keyspec *keyspec, const char *keystring)
{
	if(mbstowcs(NULL, keystring, 0) == 1) {
		wchar_t key;
		mbstowcs(&key, keystring, 1);
		keyspec->key = key;
		keyspec->iskeycode = false;
		return;
	}

	for(size_t i = 0; i < sizeof(keycodetable)/sizeof(keycodetable[0]); i++) {
		if(strcmp(keycodetable[i].string, keystring) == 0) {
			keyspec->key = keycodetable[i].keyspec.key;
			keyspec->iskeycode = keycodetable[i].keyspec.iskeycode;
			return;
		}
	}
	keyspec->key = WEOF;
}

static void keymap_entry_delete(struct keymap_entry *entry) {
	free((void*)entry->command);
	free(entry);
}

static int keymap_parse_line(struct keymap_entry *entry, char *line, struct commandexecutor *commandexecutor)
{
	char *token = line + strspn(line, " \t");
	size_t length = strcspn(token, " \t");
	if(length > 0) {
		if(token[length] == '\0')
			return EINVAL;
		token[length] = '\0';
		keymap_string_to_keyspec(&entry->keyspec, token);
		if(entry->keyspec.key == WEOF)
			return EINVAL;
	} else
		return 0;

	char *command = token + length + 1;
	command = command + strspn(command, " \t");

	int ret = commandexecutor_verify(commandexecutor, command);
	if(ret == 0) {
		entry->command = strdup(command);
		if(entry->command == NULL)
			return ENOMEM;
	} else
		return ret;

	return 0;
}

int keymap_setfromstring(struct keymap *keymap, char *keymapstring)
{
	char *saveptr;
	char *token = strtok_r(keymapstring, "\n", &saveptr);

	if(token == NULL)
		return EINVAL;

	keymap->entries = list_new(0);
	if(keymap->entries == NULL)
		return ENOMEM;

	do {
		struct keymap_entry *entry = malloc(sizeof(*entry));
		if(entry == NULL)
			return ENOMEM;

		entry->command = NULL;
		int ret = keymap_parse_line(entry, token, keymap->commandexecutor);
		if(ret != 0) {
			keymap_entry_delete(entry);
			return ret;
		}

		if(entry->command != NULL) {
			if(!list_append(keymap->entries, entry)) {
				keymap_entry_delete(entry);
				return ENOMEM;
			}
		}

	} while((token = strtok_r(NULL, "\n", &saveptr)) != NULL);

	return 0;
}

int keymap_setfromfile(struct keymap *keymap, const char *filename)
{
	struct stat st;
	int ret = stat(filename, &st);
	if(ret != 0)
		return errno;

	size_t size = st.st_size;
	char *contents = malloc(size + 1);

	if(contents == NULL)
		return ENOMEM;

	int fd = open(filename, O_RDONLY);
	if(fd < 0) {
		free(contents);
		return errno;
	}

	ret = read(fd, contents, size);
	close(fd);

	if(ret > 0) {
		contents[ret] = '\0';
		ret = keymap_setfromstring(keymap, contents);
	} else
		ret = errno;

	free(contents);
	return ret;
}

void keymap_init(struct keymap *keymap, struct commandexecutor *commandexecutor)
{
	keymap->entries = NULL;
	keymap->commandexecutor = commandexecutor;
}

void keymap_destroy(struct keymap *keymap)
{
	if(keymap == NULL)
		return;

	list_delete(keymap->entries, (list_item_deallocator)keymap_entry_delete);
}
