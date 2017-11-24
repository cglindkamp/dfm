/* See LICENSE file for copyright and license details. */
#include "commandline.h"

#include <errno.h>
#include <stdlib.h>

static int commandline_makeroom(struct commandline *commandline)
{
	size_t newsize = commandline->allocated_size * 2;
	wchar_t *newbuffer = realloc(commandline->buffer, newsize * sizeof(wchar_t));

	if(newbuffer == NULL)
		return ENOMEM;

	commandline->buffer = newbuffer;
	commandline->allocated_size = newsize;
	return 0;
}

void commandline_updatecursor(struct commandline *commandline)
{
	wmove(commandline->window, 0, commandline->cursor_pos - commandline->first + 1);
	wrefresh(commandline->window);
}

static void commandline_updateview(struct commandline *commandline)
{
	size_t length = wcslen(commandline->buffer);
	size_t promptlength = 1;
	size_t windowwidth = getmaxx(commandline->window);

	if(length < windowwidth - promptlength)
		commandline->first = 0;
	else if(commandline->cursor_pos < commandline->first)
		commandline->first = commandline->cursor_pos;
	else if(commandline->cursor_pos >= commandline->first + windowwidth - promptlength)
		commandline->first = commandline->cursor_pos - windowwidth + promptlength + 1;
	else if(wcslen(commandline->buffer + commandline->first) < windowwidth - promptlength) {
		commandline->first = length - (windowwidth - promptlength);
		if(commandline->cursor_pos == length)
			commandline->first++;
	}
	werase(commandline->window);
	mvwaddwstr(commandline->window, 0, 0, commandline->prompt);
	waddnwstr(commandline->window, commandline->buffer + commandline->first, windowwidth - promptlength);
	wrefresh(commandline->window);
	commandline_updatecursor(commandline);
}

int commandline_start(struct commandline *commandline, wchar_t prompt)
{
	commandline->prompt[0] = prompt;
	commandline->cursor_pos = 0;
	commandline->first = 0;

	commandline->allocated_size = 1;
	if(commandline->buffer == NULL)
		commandline->buffer = malloc(commandline->allocated_size * sizeof(wchar_t));
	if(commandline->buffer == NULL)
		return ENOMEM;

	commandline->buffer[0] = L'\0';

	commandline_updateview(commandline);

	return 0;
}

int commandline_handlekey(struct commandline *commandline, wint_t key, bool iskeycode)
{
	size_t length = wcslen(commandline->buffer);
	size_t cursorpos = commandline->cursor_pos;

	if(iskeycode) {
		switch(key) {
		case KEY_BACKSPACE:
			if(commandline->cursor_pos > 0) {
				wmemmove(commandline->buffer + cursorpos - 1, commandline->buffer + cursorpos, length - cursorpos);
				commandline->buffer[length - 1] = L'\0';
				commandline->cursor_pos--;
			}
			break;
		case KEY_DC:
			if(commandline->cursor_pos < length)
				wmemmove(commandline->buffer + cursorpos, commandline->buffer + cursorpos + 1, length - cursorpos);
			break;
		case KEY_LEFT:
			if(commandline->cursor_pos > 0)
				commandline->cursor_pos--;
			break;
		case KEY_RIGHT:
			if(commandline->cursor_pos < length)
				commandline->cursor_pos++;
			break;
		case KEY_HOME:
			commandline->cursor_pos = 0;
			break;
		case KEY_END:
			commandline->cursor_pos = length;
			break;
		}
	} else {
		if(length == commandline->allocated_size - 1)
			if(commandline_makeroom(commandline) != 0)
				return ENOMEM;

		wmemmove(commandline->buffer + cursorpos + 1, commandline->buffer + cursorpos, length - cursorpos);
		commandline->buffer[cursorpos] = key;
		commandline->buffer[length + 1] = L'\0';
		commandline->cursor_pos++;
	}

	commandline_updateview(commandline);
	return 0;
}

const wchar_t *commandline_getcommand(struct commandline *commandline)
{
	return commandline->buffer;
}

void commandline_resize(struct commandline *commandline, unsigned int x, unsigned int y, unsigned int width)
{
	mvwin(commandline->window, y, x);
	wresize(commandline->window, 1, width);
	if(commandline->buffer)
		commandline_updateview(commandline);
}

bool commandline_init(struct commandline *commandline, unsigned int x, unsigned int y, unsigned int width)
{
	commandline->buffer = NULL;
	commandline->prompt[1] = L'\0';
	commandline->window = newwin(1, width, y, x);
	if(commandline->window == NULL)
		return false;
	return true;
}

void commandline_destroy(struct commandline *commandline)
{
	if(commandline->window != NULL)
		delwin(commandline->window);
	free(commandline->buffer);
}
