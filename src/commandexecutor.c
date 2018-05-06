/* See LICENSE file for copyright and license details. */
#include "commandexecutor.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

static void map_command(struct command_map *commandmap, const char *commandname, command_ptr *command_ptr, bool *param_mandatory)
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

static int command_parse(char *command, command_ptr *commandptr, char **param, struct command_map *commandmap)
{
	bool param_mandatory;
	bool characters_after_command = false;

	char *token = command;
	size_t length = strcspn(token, " \t");

	if(length > 0) {
		if(token[length] != '\0')
			characters_after_command = true;
		token[length] = '\0';
		map_command(commandmap, token, commandptr, &param_mandatory);
		if(*commandptr == NULL)
			return EINVAL;
	} else
		return EINVAL;

	if(characters_after_command) {
		token = token + length + 1 + strspn(token + length + 1, " \t");
		if(token[0] != '\0') {
			*param = token;
			return 0;
		}
	}

	*param = NULL;
	if(param_mandatory)
		return EINVAL;
	return 0;
}

int commandexecutor_verify(struct commandexecutor *commandexecutor, const char *command) {
	command_ptr commandptr;
	char *param;

	char buffer[strlen(command) + 1];
	strcpy(buffer, command);

	return command_parse(buffer, &commandptr, &param, commandexecutor->commandmap);
}

int commandexecutor_execute(struct commandexecutor *commandexecutor, char *command) {
	command_ptr commandptr;
	char *param;

	int ret = command_parse(command, &commandptr, &param, commandexecutor->commandmap);
	if(ret == 0)
		commandptr(commandexecutor, param);
	else
		return ret;
	return 0;
}

void commandexecutor_init(struct commandexecutor *commandexecutor, struct command_map *commandmap)
{
	commandexecutor->commandmap = commandmap;
}

