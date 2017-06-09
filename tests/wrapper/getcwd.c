/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <string.h>

#include "getcwd.h"

static const char *cwd;
static bool should_fail;

void getcwd_setbehaviour(const char *path, bool fail)
{
	cwd = path;
	should_fail = fail;
}

char *__wrap_getcwd(char *buf, size_t size)
{
	if(should_fail) {
		errno = EACCES;
		return NULL;
	}
	if(size < strlen(cwd) + 1) {
		errno = ERANGE;
		return NULL;
	}
	strcpy(buf, cwd);
	return buf;
}
