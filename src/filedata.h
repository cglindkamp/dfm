/* See LICENSE file for copyright and license details. */
#ifndef FILEDATA_H
#define FILEDATA_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

struct filedata {
	const char *filename;
	bool is_link;
	bool is_link_broken;
	struct stat stat;
	bool is_marked;
};

int filedata_listcompare_filename(const void *a, const void *b);
int filedata_listcompare_directory_filename(const void *a, const void *b);
int filedata_listcompare_directory_filename_descending(const void *a, const void *b);
int filedata_listcompare_directory_size_filename(const void *a, const void *b);
int filedata_listcompare_directory_size_filename_descending(const void *a, const void *b);
int filedata_listcompare_directory_mtime_filename(const void *a, const void *b);
int filedata_listcompare_directory_mtime_filename_descending(const void *a, const void *b);

#define FILEDATA_FORMAT_OUTPUT_BUFFER_SIZE (sizeof("drwxrwxrwx 1970-01-01 00:00:00") + 2 * 33)
void filedata_format_output(const struct filedata *filedata, char *buffer);
size_t filedata_format_list_line(struct filedata *filedata, wchar_t *buffer, size_t len, size_t width);

int filedata_new_from_file(struct filedata **filedata, int dirfd, const char *filename);
void filedata_delete(struct filedata *filedata);

#endif
