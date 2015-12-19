#ifndef LISTMODEL_IMPL_H
#define LISTMODEL_IMPL_H

void listmodel_notify_change(struct listmodel *model, unsigned int index, enum model_change change);
void listmodel_init(struct listmodel *model);
void listmodel_free(struct listmodel *model);

#endif
