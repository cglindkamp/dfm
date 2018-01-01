/* See LICENSE file for copyright and license details. */
#ifndef DIRMODEL_H
#define DIRMODEL_H

#include "listmodel.h"

const char *dirmodel_getfilename(struct listmodel *model, size_t index);
int dirmodel_getmarkedfilenames(struct listmodel *model, const struct list **markedlist_out) __attribute__((warn_unused_result));
void dirmodel_notify_file_deleted(struct listmodel *model, const char *filename);
int dirmodel_notify_file_added_or_changed(struct listmodel *model, const char *filename);
bool dirmodel_isdir(struct listmodel *model, size_t index);
bool dirmodel_get_index(struct listmodel *model, const char *filename, size_t *index);
size_t dirmodel_regex_getnext(struct listmodel *model, const char *regex, size_t start_index, int direction);
void dirmodel_regex_setmark(struct listmodel *model, const char *regex, bool mark);
bool dirmodel_change_directory(struct listmodel *model, const char *path) __attribute__((warn_unused_result));
void dirmodel_init(struct listmodel *model);
void dirmodel_destroy(struct listmodel *model);

#endif
