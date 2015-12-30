#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev.h>

#include "dirmodel.h"
#include "listmodel_impl.h"
#include "list.h"

struct data {
	struct list *list;
	ev_io inotify_watcher;
	int inotify_fd;
	int inotify_watch;
};

struct filedata {
	char *filename;
	struct stat stat;
};

unsigned int dirmodel_count(struct listmodel *model)
{
	struct data *data = model->data;
	list_t *list = data->list;
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
	struct data *data = model->data;
	list_t *list = data->list;
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

static bool find_file_in_list(list_t *list, struct filedata *filedata, unsigned int *index)
{
	struct filedata *filedatacur;
	unsigned int min, middle, max;
	int ret;

	min = 0;
	max = list_length(list) - 1;

	while(min < max) {
		middle = (min + max) / 2;
		filedatacur = list_get_item(list, middle);
		ret = sort_filename(&filedata, &filedatacur);
		if(ret <= 0)
			max = middle;
		else
			min = middle + 1;
	}

	*index = min;
	filedatacur = list_get_item(list, *index);
	ret = sort_filename(&filedata, &filedatacur);

	if(ret > 0 && *index == list_length(list) - 1)
		*index = list_length(list);
	return ret == 0;

}

static void inotify_cb(EV_P_ ev_io *w, int revents)
{
	struct listmodel *model = w->data;
	struct data *data = model->data;
	list_t *list = data->list;
	char buf[4096]
		__attribute__((aligned(__alignof(struct inotify_event))));
	char *ptr;
	const struct inotify_event *event;
	ssize_t len;
	struct filedata *filedata, *filedataold;
	unsigned int index;
	bool found;

	len = read(data->inotify_fd, &buf, sizeof(buf));
	if(len <= 0)
		return;

	for(ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
		event = (const struct inotify_event *)ptr;

		filedata = malloc(sizeof(*filedata));
		filedata->filename = strdup(event->name);

		if(event->mask & (IN_DELETE | IN_MOVED_FROM)) {
			/* We cannot stat a deleted or moved file, but the
			 * compare function used by the search function uses
			 * S_IFDIR. So search two times, with and without the
			 * flag set */
			filedata->stat.st_mode = S_IFDIR;
			found = find_file_in_list(list, filedata, &index);
			if(!found) {
				filedata->stat.st_mode = 0;
				found = find_file_in_list(list, filedata, &index);
			}
			free(filedata->filename);
			free(filedata);
			if(found) {
				filedataold = list_get_item(list, index);
				free(filedataold->filename);
				free(filedataold);
				list_remove(list, index);
				listmodel_notify_change(model, index, MODEL_REMOVE);
			}
		} else if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) {
			stat(filedata->filename, &filedata->stat);
			found = find_file_in_list(list, filedata, &index);
			if(found) {
				filedataold = list_get_item(list, index);
				list_set_item(list, index, filedata);
				free(filedataold->filename);
				free(filedataold);
				listmodel_notify_change(model, index, MODEL_CHANGE);
			} else {
				list_insert(list, index, filedata);
				listmodel_notify_change(model, index, MODEL_ADD);
			}
		}
	}
}

const char *dirmodel_getfilename(struct listmodel *model, unsigned int index)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->filename;
}

void dirmodel_init(struct listmodel *model, const char *path)
{
	DIR *dir;
	struct dirent *entry;
	struct filedata *filedata;
	struct data *data;
	list_t *list = list_new(0);
	struct ev_loop *loop = EV_DEFAULT;

	listmodel_init(model);

	data = malloc(sizeof(*data));
	data->list = list;

	model->count = dirmodel_count;
	model->render = dirmodel_render;
	model->data = data;

	data->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	data->inotify_watch = inotify_add_watch(data->inotify_fd, ".",
		IN_CREATE |
		IN_DELETE |
		IN_MOVED_FROM |
		IN_MOVED_TO |
		IN_MODIFY |
		IN_EXCL_UNLINK
		);

	data->inotify_watcher.data = model;
	ev_io_init(&data->inotify_watcher, inotify_cb, data->inotify_fd, EV_READ);
	ev_io_start(loop, &data->inotify_watcher);

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
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata;
	struct ev_loop *loop = EV_DEFAULT;
	int i;

	ev_io_stop(loop, &data->inotify_watcher);
	inotify_rm_watch(data->inotify_fd, data->inotify_watch);
	close(data->inotify_fd);
	for(i = 0; i < list_length(list); i++) {
		filedata = list_get_item(list, i);
		free(filedata->filename);
		free(filedata);
	}
	list_free(list);
	free(data);

	listmodel_free(model);
}

