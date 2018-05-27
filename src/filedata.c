/* See LICENSE file for copyright and license details. */
#include "filedata.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int filedata_listcompare_filename(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	return strcmp(filedata1->filename, filedata2->filename);
}

int filedata_listcompare_directory_filename(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if((!S_ISDIR(filedata1->stat.st_mode) && !S_ISDIR(filedata2->stat.st_mode)) ||
	   ( S_ISDIR(filedata1->stat.st_mode) &&  S_ISDIR(filedata2->stat.st_mode)))
		return strcoll(filedata1->filename, filedata2->filename);
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

int filedata_listcompare_directory_filename_descending(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if((!S_ISDIR(filedata1->stat.st_mode) && !S_ISDIR(filedata2->stat.st_mode)) ||
	   ( S_ISDIR(filedata1->stat.st_mode) &&  S_ISDIR(filedata2->stat.st_mode)))
		return -strcoll(filedata1->filename, filedata2->filename);
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

int filedata_listcompare_directory_size_filename(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if(!S_ISDIR(filedata1->stat.st_mode) && !S_ISDIR(filedata2->stat.st_mode)) {
		if(filedata1->stat.st_size < filedata2->stat.st_size)
			return -1;
		else if(filedata1->stat.st_size > filedata2->stat.st_size)
			return 1;
		return strcoll(filedata1->filename, filedata2->filename);
	}
	if(S_ISDIR(filedata1->stat.st_mode) && S_ISDIR(filedata2->stat.st_mode))
		return strcoll(filedata1->filename, filedata2->filename);
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

int filedata_listcompare_directory_size_filename_descending(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if(!S_ISDIR(filedata1->stat.st_mode) && !S_ISDIR(filedata2->stat.st_mode)) {
		if(filedata1->stat.st_size < filedata2->stat.st_size)
			return 1;
		else if(filedata1->stat.st_size > filedata2->stat.st_size)
			return -1;
		return strcoll(filedata1->filename, filedata2->filename);
	}
	if(S_ISDIR(filedata1->stat.st_mode) && S_ISDIR(filedata2->stat.st_mode))
		return strcoll(filedata1->filename, filedata2->filename);
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

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
