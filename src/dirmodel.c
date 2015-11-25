#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "dirmodel.h"
#include "list.h"

unsigned int dirmodel_count(struct listmodel *model)
{
	list_t *list = model->data;
	return list_length(list);
}

void dirmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index)
{
	list_t *list = model->data;
	wchar_t *fmta = L"%%-%ds";
	wchar_t fmt[10];

	swprintf(fmt, 10, fmta, len);
	swprintf(buffer, len + 1, fmt, list_get_item(list, index));
}

static int sort_filename(const void *a, const void *b)
{
	return strcmp(*(char **)a, *(char **)b);
}

void dirmodel_init(struct listmodel *model, const char *path)
{
	DIR *dir;
	struct dirent *entry;
	list_t *list = list_new(0);

	model->count = dirmodel_count;
	model->render = dirmodel_render;
	model->data = list;

	dir = opendir(path);
	entry = readdir(dir);
	while(entry) {
		if(strcmp(entry->d_name, ".") != 0 &&
		   strcmp(entry->d_name, "..") != 0) {
			list_append(list, strdup(entry->d_name));
		}
		entry = readdir(dir);
	}
	closedir(dir);

	list_sort(list, sort_filename);
}

void dirmodel_free(struct listmodel *model)
{
	list_t *list = model->data;
	int i;

	for(i = 0; i < list_length(list); i++)
		free(list_get_item(list, i));
	list_free(list);
}

