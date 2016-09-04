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
#include <wchar.h>

#include <ev.h>

#include "dirmodel.h"
#include "listmodel_impl.h"
#include "list.h"

#define INFO_SEPARATOR        L" "
#define INFO_SEPARATOR_LENGTH 1
#define INFO_LINK             L"-> "
#define INFO_LINK_LENGTH      3
#define INFO_DIR              L"<DIR>"
#define INFO_SIZE_OVERFLOW    L">9000"
#define INFO_SIZE_DIR_LENGTH  5

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

static void filesize_to_string(wchar_t *buf, off_t filesize)
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
		wcscpy(buf, INFO_SIZE_OVERFLOW);
	else if(cv < 100 && cs > 0)
		swprintf(buf, 6, L"%2lu.%1lu%c", cv, (filesize - cv * 1024) * 10 / 1024, suffix[cs]);
	else
		swprintf(buf, 6, L"%4lu%c", cv, suffix[cs]);
}

static size_t render_info(wchar_t *buffer, struct filedata *filedata)
{
	size_t info_size = INFO_SIZE_DIR_LENGTH + 1;

	wcscpy(buffer, INFO_SEPARATOR);

	if(filedata->is_link) {
		info_size += INFO_LINK_LENGTH;
		wcscat(buffer, INFO_LINK);
	}

	if(S_ISDIR(filedata->stat.st_mode))
		wcscat(buffer, INFO_DIR);
	else
		filesize_to_string(buffer + wcslen(buffer), filedata->stat.st_size);

	return info_size;
}

static size_t render_filename(wchar_t *buffer, size_t len, size_t width, const char *filename)
{
	size_t wc_count = mbstowcs(NULL, filename, 0);
	wchar_t name[wc_count + 1];

	mbstowcs(name, filename, wc_count);

	size_t char_count = 0, display_count = 0;

	for(size_t i = 0; i < wc_count; i++) {
		int w = wcwidth(name[i]);
		if(w == -1)
			continue;
		if(w + display_count > width)
			break;
		if(char_count < len)
			buffer[char_count] = name[i];
		display_count += w;
		char_count++;
	}
	buffer[char_count] = L'\0';

	return char_count;
}

static size_t dirmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, size_t width, size_t index)
{
	struct data *data = model->data;
	list_t *list = data->list;
	struct filedata *filedata = list_get_item(list, index);
	wchar_t info[INFO_SEPARATOR_LENGTH + INFO_LINK_LENGTH + INFO_SIZE_DIR_LENGTH + 1];
	size_t info_size;
	size_t char_count = 0;
	size_t display_count = 0;

	info_size = render_info(info, filedata);

	if(width > info_size) {
		char_count = render_filename(buffer, len - info_size, width - info_size, filedata->filename);
		display_count = wcswidth(buffer, len);

		if(char_count + info_size > len)
			return char_count + info_size;
	} else
		info_size = width;


	if(display_count + info_size < width) {
		size_t padlength = width - display_count - info_size;

		if(char_count + padlength + info_size <= len)
			wmemset(&buffer[char_count], L' ', padlength);

		char_count += padlength;
	}
	buffer[char_count] = L'\0';

	if(char_count + info_size <= len)
		wcsncat(buffer, info + wcslen(info) - info_size, info_size);

	return char_count + info_size;
}

static int sort_filename(const void *a, const void *b)
{
	struct filedata *filedata1 = *(struct filedata **)a;
	struct filedata *filedata2 = *(struct filedata **)b;

	if((!S_ISDIR(filedata1->stat.st_mode) && !S_ISDIR(filedata2->stat.st_mode)) ||
	   ( S_ISDIR(filedata1->stat.st_mode) &&  S_ISDIR(filedata2->stat.st_mode)))
		return strcoll(filedata1->filename, filedata2->filename);
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
