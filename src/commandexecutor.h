/* See LICENSE file for copyright and license details. */
#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H

#include <stdbool.h>

struct commandexecutor;

typedef void (*command_ptr)(struct commandexecutor *, const char *);
typedef command_ptr (*command_map_ptr)(const char *);

struct command_map {
	const char *command_name;
	command_ptr command;
	bool param_mandatory;
};

struct commandexecutor {
	struct command_map *commandmap;
};

int commandexecutor_verify(struct commandexecutor *commandexecutor, const char *command);
int commandexecutor_execute(struct commandexecutor *commandexecutor, char *command);
void commandexecutor_init(struct commandexecutor *commandexecutor, struct command_map *commandmap);

#endif

