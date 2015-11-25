#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dirmodel.h"
#include "list.h"

struct filedata {
	char *filename;
	struct stat stat;
};

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
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if((!S_ISDIR(filedata1->stat.st_mode) && !S_ISDIR(filedata2->stat.st_mode)) ||
	   ( S_ISDIR(filedata1->stat.st_mode) &&  S_ISDIR(filedata2->stat.st_mode)))
		return strcmp(filedata1->filename, filedata2->filename);
	if(S_ISDIR(filedata1->stat.st_mode))
		return -1;
	return 1;
}

void dirmodel_init(struct listmodel *model, const char *path)
{
	DIR *dir;
	struct dirent *entry;
	struct filedata *filedata;
	list_t *list = list_new(0);

	model->count = dirmodel_count;
	model->render = dirmodel_render;
	model->data = list;

	dir = opendir(path);
	entry = readdir(dir);
	while(entry) {
		if(strcmp(entry->d_name, ".") != 0 &&
		   strcmp(entry->d_name, "..") != 0) {
			filedata = malloc(sizeof(*filedata));
			filedata->filename = strdup(entry->d_name);
			fstatat(dirfd(dir), filedata->filename, &filedata->stat, AT_SYMLINK_NOFOLLOW);
			list_append(list, filedata);
		}
		entry = readdir(dir);
	}
	closedir(dir);

	list_sort(list, sort_filename);
}

void dirmodel_free(struct listmodel *model)
{
	list_t *list = model->data;
	struct filedata *filedata;
	int i;

	for(i = 0; i < list_length(list); i++) {
		filedata = list_get_item(list, i);
		free(filedata->filename);
		free(filedata);
	}
	list_free(list);
}

