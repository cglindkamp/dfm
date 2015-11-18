#include "listmodel.h"

unsigned int listmodel_count(struct listmodel *model)
{
	return model->count(model);
}

void listmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index)
{
	model->render(model, buffer, len, index);
}


