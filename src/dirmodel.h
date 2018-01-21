/* See LICENSE file for copyright and license details. */
#ifndef DIRMODEL_H
#define DIRMODEL_H

#include "listmodel.h"

#include <dirent.h>

struct dirmodel {
	struct listmodel listmodel;
	struct list *list;
	DIR *dir;
};

const char *dirmodel_getfilename(struct dirmodel *model, size_t index);
int dirmodel_getmarkedfilenames(struct dirmodel *model, const struct list **markedlist_out) __attribute__((warn_unused_result));
void dirmodel_notify_file_deleted(struct dirmodel *model, const char *filename);
int dirmodel_notify_file_added_or_changed(struct dirmodel *model, const char *filename);
bool dirmodel_isdir(struct dirmodel *model, size_t index);
bool dirmodel_get_index(struct dirmodel *model, const char *filename, size_t *index);
size_t dirmodel_regex_getnext(struct dirmodel *model, const char *regex, size_t start_index, int direction);
void dirmodel_regex_setmark(struct dirmodel *model, const char *regex, bool mark);
bool dirmodel_change_directory(struct dirmodel *model, const char *path) __attribute__((warn_unused_result));
void dirmodel_init(struct dirmodel *model);
void dirmodel_destroy(struct dirmodel *model);

#endif
