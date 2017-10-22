/* See LICENSE file for copyright and license details. */
#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "list.h"

#include <sys/types.h>

struct processmanager {
	list_t *processlist;
};

int processmanager_spawn(struct processmanager *pm, const char *program, char * const argv[], const char *tmpdir, bool forground, pid_t *pid);
pid_t processmanager_waitpid(struct processmanager *pm, pid_t pid, int *wstatus);
void processmanager_init(struct processmanager *pm);
void processmanager_destroy(struct processmanager *pm);

#endif
