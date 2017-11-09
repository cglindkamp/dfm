/* See LICENSE file for copyright and license details. */
#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

struct application;

typedef void (*command_ptr)(struct application *, const char *);
typedef command_ptr (*command_map_ptr)(const char *);

struct command_map {
	const char *command_name;
	command_ptr command;
	bool param_mandatory;
};

int command_verify(const char *command, struct command_map *commandmap);
int command_execute(char *command, struct application *application, struct command_map *commandmap);

#endif

