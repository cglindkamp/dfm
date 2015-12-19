#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dirmodel.h"
#include "listmodel_impl.h"
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

static void filesize_to_string(char *buf, off_t filesize)
{
	char suffix[] = {' ', 'K', 'M', 'G', 'T', 'P', 'E'};
	int cs = 0;
	off_t cv = filesize;

	while(cv > 1024 && cs < sizeof(suffix) - 1) {
		filesize = cv;
		cv /= 1024;
		cs++;
	}

	if(cv >= 10000)
		strcpy(buf, ">9000");
	else if(cv < 100 && cs > 0)
		sprintf(buf, "%zu.%1zu%c", cv, (filesize - cv * 1024) * 10 / 1024, suffix[cs]);
	else
		sprintf(buf, "%zu%c", cv, suffix[cs]);
}

void dirmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, unsigned int index)
{
	list_t *list = model->data;
	struct filedata *filedata = list_get_item(list, index);
	wchar_t *fmta = L"%%-%d.%ds %%%ds";
	wchar_t fmt[32];
	char filesize[6];

	filesize_to_string(filesize, filedata->stat.st_size);
	swprintf(fmt, 16, fmta, len - 6, len - 6, 5);
	swprintf(buffer, len + 1, fmt, filedata->filename, filesize);
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

const char *dirmodel_getfilename(struct listmodel *model, unsigned int index)
{
	list_t *list = model->data;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->filename;
}

void dirmodel_init(struct listmodel *model, const char *path)
{
	DIR *dir;
	struct dirent *entry;
	struct filedata *filedata;
	list_t *list = list_new(0);

	listmodel_init(model);

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

	listmodel_free(model);
}

