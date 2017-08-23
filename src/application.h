/* See LICENSE file for copyright and license details. */
#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

#include "clipboard.h"
#include "list.h"
#include "listmodel.h"
#include "listview.h"
#include "path.h"

struct application {
	struct listview view;
	struct listmodel model;
	list_t *stored_positions;
	WINDOW *status;
	struct path cwd;
	struct clipboard clipboard;
	int sigwinch_fd;
	int inotify_fd;
	int inotify_watch;
	bool running;
};

bool application_init(struct application *app);
void application_destroy(struct application *app);
void application_run(struct application *app);

#endif
