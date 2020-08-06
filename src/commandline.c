/* See LICENSE file for copyright and license details. */
#include "commandline.h"

#include "list.h"

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
	commandline->history_position = 0;

	if(commandline->backup_buffer) {
		commandline->buffer = commandline->backup_buffer;
		commandline->backup_buffer = NULL;
	}
	if(commandline->buffer == NULL)
		commandline->buffer = malloc(commandline->allocated_size * sizeof(wchar_t));
	if(commandline->buffer == NULL)
		return ENOMEM;

	commandline->buffer[0] = L'\0';

	commandline_updateview(commandline);

	return 0;
}

static int commandline_copy_history_buffer(struct commandline *commandline)
{
	if(commandline->history_position == 0)
		return 0;

	wchar_t *history_buffer = commandline->buffer;
	commandline->buffer = commandline->backup_buffer;
	commandline->backup_buffer = NULL;

	while(commandline->allocated_size < wcslen(history_buffer) + 1) {
		if(commandline_makeroom(commandline) != 0)
			return ENOMEM;
	}

	wcscpy(commandline->buffer, history_buffer);
	commandline->history_position = 0;
	return 0;
}

static void commandline_delete_character_before_cursor(struct commandline *commandline)
{
	size_t oldcursorpos = commandline->cursor_pos;
	size_t length = wcslen(commandline->buffer);

	if(commandline->cursor_pos > 0) {
		size_t distance = 0;
		int width;
		do {
			width = wcwidth(commandline->buffer[commandline->cursor_pos - 1]);
			distance++;
			commandline->cursor_pos--;
		} while(commandline->cursor_pos > 0 && width < 1);

		wmemmove(commandline->buffer + commandline->cursor_pos, commandline->buffer + oldcursorpos, length - commandline->cursor_pos);
		commandline->buffer[length - distance] = L'\0';
	}
}

int commandline_handlekey(struct commandline *commandline, wint_t key, bool iskeycode)
{
	size_t length = wcslen(commandline->buffer);
	size_t cursorpos = commandline->cursor_pos;

	if(iskeycode) {
		switch(key) {
		case KEY_BACKSPACE:
			if(commandline_copy_history_buffer(commandline) != 0)
				return ENOMEM;
			commandline_delete_character_before_cursor(commandline);
			break;
		case KEY_DC:
			if(commandline_copy_history_buffer(commandline) != 0)
				return ENOMEM;
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
		case KEY_UP:
			if(commandline->history_position < list_length(commandline->history)) {
				if(commandline->history_position == 0)
					commandline->backup_buffer = commandline->buffer;
				commandline->history_position++;
				commandline->buffer = list_get_item(commandline->history, list_length(commandline->history) - commandline->history_position);
			}
			commandline->cursor_pos = wcslen(commandline->buffer);
			break;
		case KEY_DOWN:
			if(commandline->history_position > 0) {
				commandline->history_position--;
				if(commandline->history_position == 0) {
					commandline->buffer = commandline->backup_buffer;
					commandline->backup_buffer = NULL;
				} else
					commandline->buffer = list_get_item(commandline->history, list_length(commandline->history) - commandline->history_position);
			}
			commandline->cursor_pos = wcslen(commandline->buffer);
			break;
		}
	} else {
		if(key == 23) { /* Ctrl-W */
			if(commandline_copy_history_buffer(commandline) != 0)
				return ENOMEM;

			while(commandline->cursor_pos > 0 && commandline->buffer[commandline->cursor_pos - 1] == L' ') {
				commandline_delete_character_before_cursor(commandline);
			}
			while(commandline->cursor_pos > 0 && commandline->buffer[commandline->cursor_pos - 1] != L' ') {
				commandline_delete_character_before_cursor(commandline);
			}

			commandline_updateview(commandline);
			return 0;
		}

		if(key == 0 || wcwidth(key) < 0)
			return EINVAL;

		if(commandline->cursor_pos == 0 && wcwidth(key) == 0)
			return EINVAL;

		if(commandline_copy_history_buffer(commandline) != 0)
			return ENOMEM;

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

int commandline_history_add(struct commandline *commandline, const wchar_t *command)
{
	if(command == NULL)
		return EINVAL;

	/* filter out commands only consisting of whitespace */
	for(size_t i = 0; ; i++) {
		if(i == wcslen(command))
			return EINVAL;
		if(!iswspace(command[i]))
			break;
	}

	/* make old entry with same content the newest entry and ignore the new addition */
	for(size_t i = 0; i < list_length(commandline->history); i++) {
		if(wcscmp(command, list_get_item(commandline->history, i)) == 0) {
			list_move_item(commandline->history, i, list_length(commandline->history) - 1);
			return 0;
		}
	}

	wchar_t *commanddup = wcsdup(command);
	if(commanddup == NULL) {
		return ENOMEM;
	}

	if(list_append(commandline->history, commanddup))
		return 0;
	else {
		free(commanddup);
		return ENOMEM;
	}
}

bool commandline_init(struct commandline *commandline, unsigned int x, unsigned int y, unsigned int width)
{
	commandline->allocated_size = 1;
	commandline->buffer = NULL;
	commandline->backup_buffer = NULL;
	commandline->prompt[1] = L'\0';
	commandline->window = newwin(1, width, y, x);
	if(commandline->window == NULL)
		return false;
	commandline->history = list_new(0);
	if(commandline->history == NULL) {
		delwin(commandline->window);
		commandline->window = NULL;
		return false;
	}
	return true;
}

void commandline_destroy(struct commandline *commandline)
{
	if(commandline->window != NULL)
		delwin(commandline->window);
	if(commandline->backup_buffer)
		free(commandline->backup_buffer);
	else
		free(commandline->buffer);
	list_delete(commandline->history, free);
}
