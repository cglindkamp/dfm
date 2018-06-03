/* See LICENSE file for copyright and license details. */
#ifndef DIRMODEL_H
#define DIRMODEL_H

#include "listmodel.h"

#include <dirent.h>
#include <regex.h>

struct dirmodel {
	struct listmodel listmodel;
	struct list *list;
	struct list *sortedlist;
	struct list *addchange_queue;
	DIR *dir;
	regex_t filter;
	bool filter_active;
	int (*sort_compare)(const void *, const void *);
	bool sort_ascending;
};

enum dirmodel_sort_mode {
	DIRMODEL_FILENAME,
	DIRMODEL_FILENAME_DESCENDING,
	DIRMODEL_SIZE,
	DIRMODEL_SIZE_DESCENDING,
};

const char *dirmodel_getfilename(struct dirmodel *model, size_t index);
int dirmodel_getmarkedfilenames(struct dirmodel *model, const struct list **markedlist_out) __attribute__((warn_unused_result));
void dirmodel_notify_file_deleted(struct dirmodel *model, const char *filename);
int dirmodel_notify_file_added_or_changed(struct dirmodel *model, const char *filename);
int dirmodel_notify_flush(struct dirmodel *model);
bool dirmodel_isdir(struct dirmodel *model, size_t index);
bool dirmodel_get_index(struct dirmodel *model, const char *filename, size_t *index);
size_t dirmodel_regex_getnext(struct dirmodel *model, const char *regex, size_t start_index, int direction);
void dirmodel_regex_setmark(struct dirmodel *model, const char *regex, bool mark);
bool dirmodel_setfilter(struct dirmodel *model, const char *regex);
void dirmodel_set_sort_mode(struct dirmodel *model, enum dirmodel_sort_mode mode);
bool dirmodel_change_directory(struct dirmodel *model, const char *path) __attribute__((warn_unused_result));
void dirmodel_init(struct dirmodel *model);
void dirmodel_destroy(struct dirmodel *model);

#endif
