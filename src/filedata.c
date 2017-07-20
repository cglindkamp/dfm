#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "filedata.h"

int filedata_new_from_file(struct filedata **filedata, int dirfd, const char *filename)
{
	struct stat stat;

	if(fstatat(dirfd, filename, &stat, AT_SYMLINK_NOFOLLOW) != 0)
		return ENOENT;

	*filedata = malloc(sizeof(**filedata));
	if(*filedata == NULL)
		return ENOMEM;

	(*filedata)->filename = strdup(filename);
	if((*filedata)->filename == NULL) {
		free(*filedata);
		*filedata = NULL;
		return ENOMEM;
	}

	(*filedata)->is_marked = false;

	if(S_ISLNK(stat.st_mode)) {
		(*filedata)->is_link = true;
		if(fstatat(dirfd, (*filedata)->filename, &(*filedata)->stat, 0) != 0) {
			(*filedata)->is_link_broken = true;
			memcpy(&(*filedata)->stat, &stat, sizeof(stat));
		} else
			(*filedata)->is_link_broken = false;
	} else {
		(*filedata)->is_link = false;
		memcpy(&(*filedata)->stat, &stat, sizeof(stat));
	}

	return 0;
}

void filedata_delete(struct filedata *filedata)
{
	if(filedata == NULL)
		return;
	free((void *)filedata->filename);
	free(filedata);
}
