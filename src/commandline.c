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
	int cursorpos = wcswidth(commandline->buffer + commandline->first, commandline->cursor_pos - commandline->first);
	wmove(commandline->window, 0, cursorpos + 1);
	wrefresh(commandline->window);
}

static void commandline_updateview(struct commandline *commandline)
{
	size_t length = wcslen(commandline->buffer);
	int promptlength = 1;
	int windowwidth = getmaxx(commandline->window);

	if(wcswidth(commandline->buffer, -1) < windowwidth - promptlength)
		commandline->first = 0;
	else if(commandline->cursor_pos < commandline->first)
		commandline->first = commandline->cursor_pos;
	else if(wcswidth(commandline->buffer + commandline->first, commandline->cursor_pos - commandline->first) >= windowwidth - promptlength) {
		while(wcwidth(*(commandline->buffer + commandline->first)) < 1 ||
		      wcswidth(commandline->buffer + commandline->first, commandline->cursor_pos - commandline->first) >= windowwidth - promptlength) {
			commandline->first++;
		}
	} else {
		int currentwidth = wcswidth(commandline->buffer + commandline->first, commandline->cursor_pos - commandline->first + 1);
		int targetwidth = windowwidth - promptlength - (commandline->cursor_pos == length ? 1 : 0);
		int first = commandline->first;
		int firstwidth = 0;

		while(first > 0 && currentwidth + firstwidth < targetwidth) {
			currentwidth += firstwidth;

			do {
				first--;
				firstwidth = wcwidth(commandline->buffer[first]);
			} while(first > 0 && firstwidth < 1);

			if(currentwidth + firstwidth <= targetwidth)
				commandline->first = first;
		}
	}

	size_t distance = 0;
	int width = 0;
	int currentwidth = 0;
	while(commandline->first + distance < length && width + currentwidth <= windowwidth - promptlength) {
		width += currentwidth;
		currentwidth = wcwidth(*(commandline->buffer + commandline->first + distance));
		distance++;
	}

	werase(commandline->window);
	mvwaddwstr(commandline->window, 0, 0, commandline->prompt);
	waddnwstr(commandline->window, commandline->buffer + commandline->first, distance);
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
				size_t distance = 0;
				int width;
				do {
					width = wcwidth(commandline->buffer[commandline->cursor_pos - 1]);
					distance++;
					commandline->cursor_pos--;
				} while(commandline->cursor_pos > 0 && width < 1);

				wmemmove(commandline->buffer + commandline->cursor_pos, commandline->buffer + cursorpos, length - commandline->cursor_pos);
				commandline->buffer[length - distance] = L'\0';
			}
			break;
		case KEY_DC:
			if(commandline->cursor_pos < length) {
				size_t distance = 0;
				do {
					distance++;
				} while(commandline->cursor_pos + distance < length && wcwidth(commandline->buffer[commandline->cursor_pos + distance]) < 1);
				wmemmove(commandline->buffer + cursorpos, commandline->buffer + cursorpos + distance, length - cursorpos - distance + 1);
			}
			break;
		case KEY_LEFT:
			if(commandline->cursor_pos > 0) {
				int width;
				do {
					width = wcwidth(commandline->buffer[commandline->cursor_pos - 1]);
					commandline->cursor_pos--;
				} while(commandline->cursor_pos > 0 && width < 1);
			}
			break;
		case KEY_RIGHT:
			if(commandline->cursor_pos < length) {
				do {
					commandline->cursor_pos++;
				} while(commandline->cursor_pos < length && wcwidth(commandline->buffer[commandline->cursor_pos]) < 1);
			}
			break;
		case KEY_HOME:
			commandline->cursor_pos = 0;
			break;
		case KEY_END:
			commandline->cursor_pos = length;
			break;
		}
	} else {
		if(key == 0 || wcwidth(key) < 0)
			return EINVAL;

		if(commandline->cursor_pos == 0 && wcwidth(key) == 0)
			return EINVAL;

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
