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
	DIR *dir;
	bool loaded;
};

struct filedata {
	const char *filename;
	bool is_link;
	struct stat stat;
};

static void free_filedata(struct filedata *filedata)
{
	free((void *)filedata->filename);
	free(filedata);
}

size_t dirmodel_count(struct listmodel *model)
{
	struct data *data = model->data;
	list_t *list = data->list;
	return list_length(list);
}

static void filesize_to_string(char *buf, off_t filesize)
{
	char suffix[] = {' ', 'K', 'M', 'G', 'T', 'P', 'E'};
	size_t cs = 0;
	off_t cv = filesize;

	while(cv > 1024 && cs < sizeof(suffix) - 1) {
		filesize = cv;
		cv /= 1024;
		cs++;
	}

	if(cv >= 10000)
		strcpy(buf, ">9000");
	else if(cv < 100 && cs > 0)
		sprintf(buf, "%lu.%1lu%c", cv, (filesize - cv * 1024) * 10 / 1024, suffix[cs]);
	else
		sprintf(buf, "%lu%c", cv, suffix[cs]);
}

void dirmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, size_t index)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata = list_get_item(list, index);
	char filesize[6];
	char *info;
	char *link;
	size_t filename_length = len - 6;

	if(S_ISDIR(filedata->stat.st_mode)) {
		info = "<DIR>";
	} else {
		info = filesize;
		filesize_to_string(filesize, filedata->stat.st_size);
	}

	if(filedata->is_link) {
		filename_length -= 3;
		link = " ->";
	} else
		link = "";

	swprintf(buffer, len + 1, L"%-*.*s%s %*s", filename_length, filename_length, filedata->filename, link, 5, info);
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

static bool read_file_data(int dirfd, struct filedata *filedata)
{
	if(fstatat(dirfd, filedata->filename, &filedata->stat, AT_SYMLINK_NOFOLLOW) != 0)
		return false;
	filedata->is_link = false;
	if(S_ISLNK(filedata->stat.st_mode)) {
		filedata->is_link = true;
		fstatat(dirfd, filedata->filename, &filedata->stat, 0);
	}
	return true;
}

bool dirmodel_get_index(struct listmodel *model, const char *filename, size_t *index)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata filedata;
	bool found;

	filedata.filename = filename;

	/* We cannot stat a potentially deleted or moved file, but the compare function
	 * used by the search function uses S_IFDIR. So search two times, with and
	 * without the flag set */
	filedata.stat.st_mode = S_IFDIR;
	found = list_find_item_or_insertpoint(list, sort_filename, &filedata, index);
	if(!found) {
		filedata.stat.st_mode = 0;
		found = list_find_item_or_insertpoint(list, sort_filename, &filedata, index);
	}
	return found;
}

static void inotify_cb(EV_P_ ev_io *w, int revents)
{
#ifdef EV_MULTIPLICITY
	(void)loop;
#endif
	(void)revents;

	struct listmodel *model = w->data;
	struct data *data = model->data;
	list_t *list = data->list;
	char buf[4096]
		__attribute__((aligned(__alignof(struct inotify_event))));
	char *ptr;
	const struct inotify_event *event;
	ssize_t len;
	struct filedata *filedata, *filedataold;
	size_t index;
	bool found;

	len = read(data->inotify_fd, &buf, sizeof(buf));
	if(len <= 0)
		return;

	for(ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
		event = (const struct inotify_event *)ptr;

		if(event->mask & (IN_DELETE | IN_MOVED_FROM)) {
			found = dirmodel_get_index(model, event->name, &index);
			if(found) {
				filedataold = list_get_item(list, index);
				free_filedata(filedataold);
				list_remove(list, index);
				listmodel_notify_change(model, index, MODEL_REMOVE);
			}
		} else if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) {
			filedata = malloc(sizeof(*filedata));
			filedata->filename = strdup(event->name);

			if(!read_file_data(dirfd(data->dir), filedata)) {
				free_filedata(filedata);
				continue;
			}
			found = list_find_item_or_insertpoint(list, sort_filename, filedata, &index);
			if(found) {
				filedataold = list_get_item(list, index);
				list_set_item(list, index, filedata);
				free_filedata(filedataold);
				listmodel_notify_change(model, index, MODEL_CHANGE);
			} else {
				list_insert(list, index, filedata);
				listmodel_notify_change(model, index, MODEL_ADD);
			}
		}
	}
}

const char *dirmodel_getfilename(struct listmodel *model, size_t index)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->filename;
}

bool dirmodel_isdir(struct listmodel *model, size_t index)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata = list_get_item(list, index);
	return S_ISDIR(filedata->stat.st_mode);
}

static bool internal_init(struct listmodel *model, const char *path)
{
	DIR *dir;
	struct dirent *entry;
	struct filedata *filedata;
	struct data *data = model->data;
	list_t *list = list_new(0);
	struct ev_loop *loop = EV_DEFAULT;

	data->list = list;

	dir = opendir(path);
	if(dir == NULL)
		return false;

	data->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if(data->inotify_fd == -1) {
		closedir(dir);
		return false;
	}

	data->inotify_watch = inotify_add_watch(data->inotify_fd, path,
		IN_CREATE |
		IN_DELETE |
		IN_MOVED_FROM |
		IN_MOVED_TO |
		IN_MODIFY |
		IN_EXCL_UNLINK
		);

	if(data->inotify_watch == -1) {
		close(data->inotify_fd);
		closedir(dir);
		return false;
	}

	data->inotify_watcher.data = model;
	ev_io_init(&data->inotify_watcher, inotify_cb, data->inotify_fd, EV_READ);
	ev_io_start(loop, &data->inotify_watcher);

	entry = readdir(dir);
	while(entry) {
		if(strcmp(entry->d_name, ".") != 0 &&
		   strcmp(entry->d_name, "..") != 0) {
			filedata = malloc(sizeof(*filedata));
			filedata->filename = strdup(entry->d_name);
			if(!read_file_data(dirfd(dir), filedata)) {
				free_filedata(filedata);
				continue;
			}
			list_append(list, filedata);
		}
		entry = readdir(dir);
	}
	data->dir = dir;

	list_sort(list, sort_filename);

	data->loaded = true;
	return true;
}

static void internal_free(struct listmodel *model)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata;
	struct ev_loop *loop = EV_DEFAULT;
	size_t i;

	if(!data->loaded)
		return;

	ev_io_stop(loop, &data->inotify_watcher);
	inotify_rm_watch(data->inotify_fd, data->inotify_watch);
	close(data->inotify_fd);
	closedir(data->dir);
	for(i = 0; i < list_length(list); i++) {
		filedata = list_get_item(list, i);
		free_filedata(filedata);
	}
	list_free(list);
	data->loaded = false;
}

bool dirmodel_change_directory(struct listmodel *model, const char *path)
{
	internal_free(model);
	if(!internal_init(model, path))
		return false;
	listmodel_notify_change(model, 0, MODEL_RELOAD);
	return true;
}

void dirmodel_init(struct listmodel *model)
{
	listmodel_init(model);

	model->data = malloc(sizeof(struct data));
	model->count = dirmodel_count;
	model->render = dirmodel_render;

	struct data *data = model->data;
	data->loaded = false;
}

void dirmodel_free(struct listmodel *model)
{
	internal_free(model);
	free(model->data);
	listmodel_free(model);
}
