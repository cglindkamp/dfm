#include "testmodel.h"

unsigned int testmodel_count(struct listmodel *model)
{
	return 200;
}

void testmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index)
{
	wchar_t *fmta = L"%%-%dd";
	wchar_t fmt[10];

	swprintf(fmt, 10, fmta, len);
	swprintf(buffer, len + 1, fmt, index);
}

void testmodel_init(struct listmodel *model)
{
	model->count = testmodel_count;
	model->render = testmodel_render;
}

