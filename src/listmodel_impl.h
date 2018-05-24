/* See LICENSE file for copyright and license details. */
#ifndef LISTMODEL_IMPL_H
#define LISTMODEL_IMPL_H

void listmodel_notify_change(struct listmodel *model, enum model_change change, size_t newindex, size_t oldindex);
void listmodel_init(struct listmodel *model);
void listmodel_destroy(struct listmodel *model);

#endif
