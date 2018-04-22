#include "keymap.h"

#include "command.h"

#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int keymap_handlekey(struct keymap *keymap, struct application *application, wint_t key, bool iskeycode, struct command_map *commandmap)
{
	struct keymap_entry *curitem;

	for(curitem = keymap->entries; curitem->command != NULL; curitem++) {
		if(curitem->keyspec.key == key && curitem->keyspec.iskeycode == iskeycode) {
			char buffer[strlen(curitem->command) + 1];
			strcpy(buffer, curitem->command);

			return command_execute(buffer, application, commandmap);
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

static int keymap_parse_line(struct keymap_entry *entry, char *line, struct command_map *commandmap)
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

	int ret = command_verify(command, commandmap);
	if(ret == 0) {
		entry->command = strdup(command);
		if(entry->command == NULL)
			return ENOMEM;
	} else
		return ret;

	return 0;
}

int keymap_setfromstring(struct keymap *keymap, char *keymapstring, struct command_map *commandmap)
{
	size_t parsed_lines = 0;
	char *saveptr;
	char *token = strtok_r(keymapstring, "\n", &saveptr);

	if(token == NULL)
		return EINVAL;

	keymap->entries = malloc(sizeof(keymap->entries[0]));
	if(keymap->entries == NULL)
		return ENOMEM;

	do {
		int ret = keymap_parse_line(&(keymap->entries[parsed_lines]), token, commandmap);
		if(ret != 0) {
			keymap->entries[parsed_lines].command = NULL;
			keymap_destroy(keymap);
			return ret;
		}

		if(keymap->entries[parsed_lines].command != NULL) {
			parsed_lines++;

			struct keymap_entry *newentries = realloc(keymap->entries, sizeof(keymap->entries[0]) * (parsed_lines + 1));
			if(newentries == NULL) {
				free((char*)keymap->entries[parsed_lines - 1].command);
				keymap->entries[parsed_lines - 1].command = NULL;
				keymap_destroy(keymap);
				return ENOMEM;
			}
			keymap->entries = newentries;

			keymap->entries[parsed_lines].command = NULL;
		}

	} while((token = strtok_r(NULL, "\n", &saveptr)) != NULL);

	return 0;
}

int keymap_setfromfile(struct keymap *keymap, const char *filename, struct command_map *commandmap)
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
		ret = keymap_setfromstring(keymap, contents, commandmap);
	} else
		ret = errno;

	free(contents);
	return ret;
}

void keymap_init(struct keymap *keymap)
{
	keymap->entries = NULL;
}

void keymap_destroy(struct keymap *keymap)
{
	if(keymap == NULL)
		return;

	if(keymap->entries == NULL)
		return;

	for(struct keymap_entry *curitem = keymap->entries; curitem->command != NULL; curitem++)
		free((char*)curitem->command);

	free(keymap->entries);
}
