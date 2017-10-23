#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "keymap.h"

void keymap_handlekey(struct keymap *keymap, struct application *application, wint_t key, bool iskeycode)
{
	struct keymap *curitem;

	for(curitem = keymap; curitem->command != NULL; curitem++) {
		if(curitem->keyspec.key == key && curitem->keyspec.iskeycode == iskeycode) {
			curitem->command(application, curitem->param);
			break;
		}
	}
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

void keymap_map_command(struct command_map *commandmap, const char *commandname, command_ptr *command_ptr, bool *param_mandatory)
{
	*command_ptr = NULL;

	for(size_t i = 0; commandmap[i].command_name != NULL; i++) {
		if(strcmp(commandmap[i].command_name, commandname) == 0) {
			*command_ptr = commandmap[i].command;
			*param_mandatory = commandmap[i].param_mandatory;
			break;
		}
	}
}

static int keymap_parse_line(struct keymap *keymap, char *line, struct command_map *commandmap)
{
	bool param_mandatory;
	char *saveptr;
	char *token = strtok_r(line, " \t", &saveptr);
	if(token) {
		keymap_string_to_keyspec(&keymap->keyspec, token);
		if(keymap->keyspec.key == WEOF)
			return EINVAL;
	} else
		return 0;

	token = strtok_r(NULL, " \t", &saveptr);
	if(token) {
		keymap_map_command(commandmap, token, &keymap->command, &param_mandatory);
		if(keymap->command == NULL)
			return EINVAL;
	} else
		return EINVAL;

	/* The following code assumes that saveptr points to the character
	 * after the previous token or to the null terminator of line when
	 * there are no more characters. This is not guaranteed by POSIX but is
	 * implemented this way in glibc, musl and dietlibc. */
	size_t skip = strspn(saveptr, " \t");
	token = saveptr + skip;
	if(token[0] != '\0') {
		keymap->param = strdup(token);
		if(keymap->param == NULL)
			return ENOMEM;
	} else {
		keymap->param = NULL;
		if(param_mandatory)
			return EINVAL;
	}

	return 0;
}

int keymap_newfromstring(struct keymap **keymap, char *keymapstring, struct command_map *commandmap)
{
	size_t parsed_lines = 0;
	char *saveptr;
	char *token = strtok_r(keymapstring, "\n", &saveptr);

	if(token == NULL)
		return EINVAL;

	*keymap = malloc(sizeof(struct keymap));
	if(*keymap == NULL)
		return ENOMEM;

	do {
		int ret = keymap_parse_line(&((*keymap)[parsed_lines]), token, commandmap);
		if(ret != 0) {
			(*keymap)[parsed_lines].command = NULL;
			keymap_delete(*keymap);
			return ret;
		}

		if((*keymap)[parsed_lines].command != NULL) {
			parsed_lines++;

			struct keymap *newkeymap = realloc(*keymap, sizeof(struct keymap) * (parsed_lines + 1));
			if(newkeymap == NULL) {
				free((char*)(*keymap)[parsed_lines - 1].param);
				(*keymap)[parsed_lines - 1].command = NULL;
				keymap_delete(*keymap);
				return ENOMEM;
			}
			*keymap = newkeymap;

			(*keymap)[parsed_lines].command = NULL;
		}

	} while((token = strtok_r(NULL, "\n", &saveptr)) != NULL);

	return 0;
}

int keymap_newfromfile(struct keymap **keymap, const char *filename, struct command_map *commandmap)
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
		ret = keymap_newfromstring(keymap, contents, commandmap);
	} else
		ret = errno;

	free(contents);
	return ret;
}

void keymap_delete(struct keymap *keymap)
{
	if(keymap == NULL)
		return;

	for(struct keymap *curitem = keymap; curitem->command != NULL; curitem++)
		free((char*)curitem->param);

	free(keymap);
}
