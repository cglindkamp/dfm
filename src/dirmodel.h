#ifndef DIRMODEL_H
#define DIRMODEL_H

#include "listmodel.h"

const char *dirmodel_getfilename(struct listmodel *model, size_t index);
bool dirmodel_isdir(struct listmodel *model, size_t index);
bool dirmodel_get_index(struct listmodel *model, const char *filename, size_t *index);
bool dirmodel_change_directory(struct listmodel *model, const char *path);
void dirmodel_init(struct listmodel *model);
void dirmodel_free(struct listmodel *model);

#endif
