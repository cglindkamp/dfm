/* See LICENSE file for copyright and license details. */
#include "processmanager.h"

#include "list.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct process {
	pid_t pid;
	const char *tmpdir;
};

void process_delete(struct process *process)
{
	free((void*)process->tmpdir);
	free(process);
}

int processmanager_spawn(struct processmanager *pm, const char *program, char * const argv[], const char *tmpdir, bool foreground, pid_t *pid)
{
	struct process *process = NULL;

	if(tmpdir) {
		if(!pm->processlist) {
			pm->processlist = list_new(0);
			if(!pm->processlist)
				return ENOMEM;
		}

		process = malloc(sizeof(*process));
		if(!process)
			return ENOMEM;

		process->tmpdir = strdup(tmpdir);
		if(!process->tmpdir) {
			free(process);
			return ENOMEM;
		}

		if(!list_append(pm->processlist, process)) {
			process_delete(process);
			return ENOMEM;
		}
	}

	*pid = fork();

	if(*pid < 0)
		return errno;
	if(*pid > 0) {
		if(process)
			process->pid = *pid;
		return 0;
	}

	if(!foreground) {
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
	}

	execvp(program, argv);
	exit(EXIT_FAILURE);
}

pid_t processmanager_waitpid(struct processmanager *pm, pid_t pid, int *wstatus)
{
	pid = waitpid(pid, wstatus, 0);
	if(pid <= 0)
		return pid;

	if(!pm->processlist)
		return pid;

	for(size_t i = 0; i < list_length(pm->processlist); i++) {
		struct process *process = list_get_item(pm->processlist, i);

		if(process->pid == pid) {
			remove_directory_recursively(process->tmpdir);
			process_delete(process);
			list_remove(pm->processlist, i);
			break;
		}
	}

	return pid;
}

void processmanager_init(struct processmanager *pm)
{
	pm->processlist = NULL;
}

void processmanager_destroy(struct processmanager *pm)
{
	if(pm->processlist)
		list_delete(pm->processlist, (list_item_deallocator)process_delete);
}
