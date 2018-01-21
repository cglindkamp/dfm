/* See LICENSE file for copyright and license details. */
#ifndef APPLICATION_H
#define APPLICATION_H

#include "clipboard.h"
#include "commandline.h"
#include "dirmodel.h"
#include "listmodel.h"
#include "listview.h"
#include "path.h"
#include "processmanager.h"

#include <ncurses.h>
#include <stdbool.h>

struct list;

enum mode {
	MODE_NORMAL,
	MODE_COMMAND,
};

struct application {
	struct listview view;
	struct dirmodel model;
	struct list *stored_positions;
	WINDOW *status;
	struct commandline commandline;
	struct path cwd;
	struct clipboard clipboard;
	struct keymap *keymap;
	struct processmanager pm;
	int signal_fd;
	int inotify_fd;
	int inotify_watch;
	enum mode mode;
	bool running;
	const char *lastsearch_regex;
	int lastsearch_direction;
};

void application_run(struct application *app);

bool application_init(struct application *app);
void application_destroy(struct application *app);

#endif
