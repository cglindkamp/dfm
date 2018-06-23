/* See LICENSE file for copyright and license details. */
#include "filedata.h"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

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

	if(!(S_ISDIR(filedata1->stat.st_mode) ^ S_ISDIR(filedata2->stat.st_mode)))
		return strcoll(filedata1->filename, filedata2->filename);
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

int filedata_listcompare_directory_filename_descending(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if(!(S_ISDIR(filedata1->stat.st_mode) ^ S_ISDIR(filedata2->stat.st_mode)))
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

int filedata_listcompare_directory_mtime_filename(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if(!(S_ISDIR(filedata1->stat.st_mode) ^ S_ISDIR(filedata2->stat.st_mode))) {
		if(filedata1->stat.st_mtime < filedata2->stat.st_mtime)
			return -1;
		else if(filedata1->stat.st_mtime > filedata2->stat.st_mtime)
			return 1;
		return strcoll(filedata1->filename, filedata2->filename);
	}
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

int filedata_listcompare_directory_mtime_filename_descending(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if(!(S_ISDIR(filedata1->stat.st_mode) ^ S_ISDIR(filedata2->stat.st_mode))) {
		if(filedata1->stat.st_mtime < filedata2->stat.st_mtime)
			return 1;
		else if(filedata1->stat.st_mtime > filedata2->stat.st_mtime)
			return -1;
		return strcoll(filedata1->filename, filedata2->filename);
	}
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

static char filetype_character(const struct stat *statbuf)
{
	switch(statbuf->st_mode & S_IFMT) {
	case S_IFBLK:
		return 'b';
	case S_IFCHR:
		return 'b';
	case S_IFDIR:
		return 'd';
	case S_IFIFO:
		return 'p';
	case S_IFLNK:
		return 'l';
	case S_IFSOCK:
		return 's';
	case S_IFREG:
	default:
		return '-';
	}
}

static void permission_characters(char *buffer, char mode)
{
	if(mode & 4)
		buffer[0] = 'r';
	else
		buffer[0] = '-';
	if(mode & 2)
		buffer[1] = 'w';
	else
		buffer[1] = '-';
	if(mode & 1)
		buffer[2] = 'x';
	else
		buffer[2] = '-';
}

void filedata_format_output(const struct filedata *filedata, char *buffer)
{
	buffer[0] = filetype_character(&filedata->stat);
	permission_characters(buffer + 1, (filedata->stat.st_mode >> 6) & 7);
	permission_characters(buffer + 4, (filedata->stat.st_mode >> 3) & 7);
	permission_characters(buffer + 7,  filedata->stat.st_mode       & 7);
	buffer[10] = ' ';

	size_t length = 11;

	struct passwd *passwd = getpwuid(filedata->stat.st_uid);
	struct group *group = getgrgid(filedata->stat.st_gid);
	struct tm modification_time;
	localtime_r(&filedata->stat.st_mtime, &modification_time);

	if(passwd)
		length += sprintf(buffer + length, "%.32s ", passwd->pw_name);
	else
		length += sprintf(buffer + length, "%.32d ", filedata->stat.st_uid);
	if(group)
		length += sprintf(buffer + length, "%.32s ", group->gr_name);
	else
		length += sprintf(buffer + length, "%.32d ", filedata->stat.st_gid);

	length += strftime(buffer + length, sizeof("1970-01-01 00:00:00"), "%F %T", &modification_time);

	buffer[length] = 0;
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
