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
#include <wchar.h>

#define INFO_SEPARATOR        L" "
#define INFO_SEPARATOR_LENGTH 1
#define INFO_LINK             L"-> "
#define INFO_LINK_BROKEN      L"-X "
#define INFO_LINK_LENGTH      3
#define INFO_DIR              L"<DIR>"
#define INFO_SIZE_OVERFLOW    L">9000"

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

void filesize_to_string(wchar_t *buf, off_t filesize)
{
	char suffix[] = {' ', 'K', 'M', 'G', 'T', 'P', 'E'};
	size_t cs = 0;
	off_t cv = filesize;

	while(cv > 1024 && cs < sizeof(suffix) - 1) {
		filesize = cv;
		cv /= 1024;
		cs++;
	}

	if(cv >= 10000)
		wcscpy(buf, INFO_SIZE_OVERFLOW);
	else if(cv < 100 && cs > 0)
		swprintf(buf, 6, L"%lu.%1lu%c", cv, (filesize - cv * 1024) * 10 / 1024, suffix[cs]);
	else
		swprintf(buf, 6, L"%lu%c", cv, suffix[cs]);
}

static size_t render_info(wchar_t *buffer, struct filedata *filedata)
{
	size_t info_size = INFO_SIZE_DIR_LENGTH + 1;

	wcscpy(buffer, INFO_SEPARATOR);

	if(filedata->is_link) {
		info_size += INFO_LINK_LENGTH;
		if(filedata->is_link_broken)
			wcscat(buffer, INFO_LINK_BROKEN);
		else
			wcscat(buffer, INFO_LINK);
	}

	if(S_ISDIR(filedata->stat.st_mode))
		wcscat(buffer, INFO_DIR);
	else {
		wchar_t size[INFO_SIZE_DIR_LENGTH + 1];
		filesize_to_string(size, filedata->stat.st_size);
		swprintf(buffer + wcslen(buffer), sizeof(size)/sizeof(size[0]), L"%5ls", size);
	}

	return info_size;
}

static size_t render_filename(wchar_t *buffer, size_t len, size_t width, const char *filename)
{
	size_t wc_count = mbstowcs(NULL, filename, 0);

	if(wc_count == (size_t)-1) {
		buffer[0] = L'\0';
		if(len > 3)
			len = 3;
		wcsncat(buffer, L"???", len);
		return wcslen(buffer);
	}

	wchar_t name[wc_count + 1];

	mbstowcs(name, filename, wc_count);

	size_t char_count = 0, display_count = 0;

	for(size_t i = 0; i < wc_count; i++) {
		int w = wcwidth(name[i]);
		if(w == -1)
			continue;
		if(w + display_count > width)
			break;
		if(char_count < len)
			buffer[char_count] = name[i];
		display_count += w;
		char_count++;
	}
	buffer[char_count] = L'\0';

	return char_count;
}

size_t filedata_format_list_line(struct filedata *filedata, wchar_t *buffer, size_t len, size_t width)
{
	wchar_t info[INFO_SEPARATOR_LENGTH + INFO_LINK_LENGTH + INFO_SIZE_DIR_LENGTH + 1];
	size_t info_size;
	size_t char_count = 0;
	size_t display_count = 0;

	info_size = render_info(info, filedata);

	if(width > info_size) {
		char_count = render_filename(buffer, len - info_size, width - info_size, filedata->filename);
		if(char_count + info_size > len)
			return char_count + info_size;

		display_count = wcswidth(buffer, len);
	} else
		info_size = width;


	if(display_count + info_size < width) {
		size_t padlength = width - display_count - info_size;

		if(char_count + padlength + info_size <= len)
			wmemset(&buffer[char_count], L' ', padlength);

		char_count += padlength;
	}
	buffer[char_count] = L'\0';

	if(char_count + info_size <= len)
		wcsncat(buffer, info + wcslen(info) - info_size, info_size);

	return char_count + info_size;
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
		(*filedata)->link_size = stat.st_size;
		if(fstatat(dirfd, (*filedata)->filename, &(*filedata)->stat, 0) != 0) {
			(*filedata)->is_link_broken = true;
			memcpy(&(*filedata)->stat, &stat, sizeof(stat));
		} else
			(*filedata)->is_link_broken = false;
	} else {
		(*filedata)->is_link = false;
		memcpy(&(*filedata)->stat, &stat, sizeof(stat));
	}

	/* st_size field is not used for directories, so zero it out to get
	 * better file size count statistics in dirmodel */
	if(S_ISDIR((*filedata)->stat.st_mode))
		(*filedata)->stat.st_size = 0;
	return 0;
}

void filedata_delete(struct filedata *filedata)
{
	if(filedata == NULL)
		return;
	free((void *)filedata->filename);
	free(filedata);
}
