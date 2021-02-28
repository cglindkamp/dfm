/* See LICENSE file for copyright and license details. */
#include "fstatat.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __USE_EXTERN_INLINES
#error fstatat in glibc can only be wrapped, if __USE_EXTERN_INLINES is not set. Use -O0 or -Os
#endif

int __real_fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);

static int errno_value = 0;

void fstatat_seterrno(int forced_errno)
{
	errno_value = forced_errno;
}

int __wrap_fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
	int ret = __real_fstatat(dirfd, pathname, statbuf, flags);

	if(errno_value == 0) {
		return ret;
	}
	errno = errno_value;
	return -1;
}
