#ifndef DIRMODEL_H
#define DIRMODEL_H

#include "listmodel.h"

void dirmodel_init(struct listmodel *model, const char *path);
void dirmodel_free(struct listmodel *model);

#endif
