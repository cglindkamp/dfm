/* See LICENSE file for copyright and license details. */
#ifndef FILEDATA_H
#define FILEDATA_H

#include <stdbool.h>
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

int filedata_new_from_file(struct filedata **filedata, int dirfd, const char *filename);
void filedata_delete(struct filedata *filedata);

#endif
