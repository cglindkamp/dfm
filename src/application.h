/* See LICENSE file for copyright and license details. */
#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

#include "clipboard.h"
#include "list.h"
#include "listmodel.h"
#include "listview.h"
#include "path.h"
#include "processmanager.h"

struct application {
	struct listview view;
	struct listmodel model;
	list_t *stored_positions;
	WINDOW *status;
	struct path cwd;
	struct clipboard clipboard;
	struct keymap *keymap;
	struct processmanager pm;
	int signal_fd;
	int inotify_fd;
	int inotify_watch;
	bool running;
};

void application_run(struct application *app);

bool application_init(struct application *app);
void application_destroy(struct application *app);

#endif
