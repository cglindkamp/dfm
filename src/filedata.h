#ifndef FILEDATA_H
#define FILEDATA_H

#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

struct filedata {
	const char *filename;
	bool is_link;
	bool is_link_broken;
	struct stat stat;
	bool is_marked;
};

int filedata_new_from_file(struct filedata **filedata, int dirfd, const char *filename);
void filedata_delete(struct filedata *filedata);

#endif