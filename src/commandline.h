/* See LICENSE file for copyright and license details. */
#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <ncurses.h>
#include <stdbool.h>
#include <wchar.h>

struct commandline {
	WINDOW *window;
	wchar_t *buffer;
	size_t allocated_size;
	wchar_t prompt[2];
	size_t cursor_pos;
	size_t first;
};

int commandline_start(struct commandline *commandline, wchar_t prompt);
void commandline_updatecursor(struct commandline *commandline);
int commandline_handlekey(struct commandline *commandline, wint_t key, bool iskeycode);
const wchar_t *commandline_getcommand(struct commandline *commandline);
void commandline_resize(struct commandline *commandline, unsigned int x, unsigned int y, unsigned int width);

bool commandline_init(struct commandline *commandline, unsigned int x, unsigned int y, unsigned int width);
void commandline_destroy(struct commandline *commandline);

#endif
