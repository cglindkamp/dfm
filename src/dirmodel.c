/* See LICENSE file for copyright and license details. */
#include "dirmodel.h"

#include "filedata.h"
#include "listmodel_impl.h"
#include "list.h"
#include "util.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <wchar.h>

#define INFO_SEPARATOR        L" "
#define INFO_SEPARATOR_LENGTH 1
#define INFO_LINK             L"-> "
#define INFO_LINK_BROKEN      L"-X "
#define INFO_LINK_LENGTH      3
#define INFO_DIR              L"<DIR>"
#define INFO_SIZE_OVERFLOW    L">9000"
#define INFO_SIZE_DIR_LENGTH  5

size_t dirmodel_count(struct listmodel *listmodel)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);
	struct list *list = model->list;
	if(list == NULL)
		return 0;
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
		if(filedata->is_link_broken)
			wcscat(buffer, INFO_LINK_BROKEN);
		else
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

	if(wc_count == (size_t)-1) {
		buffer[0] = L'\0';
		if(len > 3)
			len = 3;
		wcsncat(buffer, L"???", len);
		return wcslen(buffer);
	}

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

static size_t dirmodel_render(struct listmodel *listmodel, wchar_t *buffer, size_t len, size_t width, size_t index)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);;
	struct list *list = model->list;
	struct filedata *filedata = list_get_item(list, index);
	wchar_t info[INFO_SEPARATOR_LENGTH + INFO_LINK_LENGTH + INFO_SIZE_DIR_LENGTH + 1];
	size_t info_size;
	size_t char_count = 0;
	size_t display_count = 0;

	info_size = render_info(info, filedata);

	if(width > info_size) {
		char_count = render_filename(buffer, len - info_size, width - info_size, filedata->filename);
		if(char_count + info_size > len)
			return char_count + info_size;

		display_count = wcswidth(buffer, len);
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

static void dirmodel_setmark(struct listmodel *listmodel, size_t index, bool mark)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);;
	struct list *list = model->list;
	struct filedata *filedata = list_get_item(list, index);
	filedata->is_marked = mark;
	listmodel_notify_change(listmodel, index, MODEL_CHANGE);
}

static bool dirmodel_ismarked(struct listmodel *listmodel, size_t index)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);;
	struct list *list = model->list;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->is_marked;
}

int dirmodel_getmarkedfilenames(struct dirmodel *model, const struct list **markedlist_out)
{
	struct list *list = model->list;
	struct list *markedlist = list_new(0);

	if(markedlist == NULL)
		return ENOMEM;

	size_t length = list_length(list);

	for(size_t i = 0; i < length; i++)
	{
		struct filedata *filedata = list_get_item(list, i);
		if(filedata->is_marked) {
			char *filename = strdup(filedata->filename);
			if(filename == NULL) {
				list_delete(markedlist, free);
				return ENOMEM;
			}
			if(!list_append(markedlist, filename)) {
				free(filename);
				list_delete(markedlist, free);
				return ENOMEM;
			}
		}
	}
	if(list_length(markedlist) == 0) {
		list_delete(markedlist, NULL);
		return ENOENT;
	}

	*markedlist_out = markedlist;
	return 0;
}

bool dirmodel_get_index(struct dirmodel *model, const char *filename, size_t *index)
{
	struct list *list = model->list;
	struct filedata filedata;
	bool found;

	filedata.filename = filename;

	/* We cannot stat a potentially deleted or moved file, but the compare function
	 * used by the search function uses S_IFDIR. So search two times, with and
	 * without the flag set */
	filedata.stat.st_mode = S_IFDIR;
	found = list_find_item_or_insertpoint(list, filedata_listcompare_directory_filename, &filedata, index);
	if(!found) {
		filedata.stat.st_mode = 0;
		found = list_find_item_or_insertpoint(list, filedata_listcompare_directory_filename, &filedata, index);
	}
	return found;
}

size_t dirmodel_regex_getnext(struct dirmodel *model, const char *regex, size_t start_index, int direction)
{
	struct list *list = model->list;
	regex_t cregex;
	size_t result = start_index;

	int ret = regcomp(&cregex, regex, REG_EXTENDED | REG_ICASE | REG_NOSUB);
	if(ret != 0)
		return start_index;

	if(direction > 0 && start_index < SIZE_MAX) {
		size_t length = list_length(list);
		for(size_t i = start_index + 1; i < length; i++) {
			struct filedata *filedata = list_get_item(list, i);
			if(regexec(&cregex, filedata->filename, 0, NULL, 0) == 0) {
				result = i;
				break;
			}
		}
	} else if(direction < 0 && start_index > 0) {
		for(size_t i = start_index - 1; i > 0; i--) {
			struct filedata *filedata = list_get_item(list, i);
			if(regexec(&cregex, filedata->filename, 0, NULL, 0) == 0) {
				result = i;
				break;
			}
		}
	}

	regfree(&cregex);
	return result;
}

void dirmodel_regex_setmark(struct dirmodel *model, const char *regex, bool mark)
{
	struct list *list = model->list;
	regex_t cregex;

	int ret = regcomp(&cregex, regex, REG_EXTENDED | REG_ICASE | REG_NOSUB);
	if(ret != 0)
		return;

	size_t length = list_length(list);

	for(size_t i = 0; i < length; i++) {
		struct filedata *filedata = list_get_item(list, i);
		if(regexec(&cregex, filedata->filename, 0, NULL, 0) == 0) {
			filedata->is_marked = mark;
			listmodel_notify_change(&model->listmodel, i, MODEL_CHANGE);
		}
	}

	regfree(&cregex);
}

bool dirmodel_setfilter(struct dirmodel *model, const char *regex)
{
	if(model->filter_active)
		regfree(&model->filter);

	if(regex == NULL) {
		model->filter_active = false;
	} else {
		int ret = regcomp(&model->filter, regex, REG_EXTENDED | REG_ICASE | REG_NOSUB);
		model->filter_active = (ret == 0);
	}

	return model->filter_active;
}

void dirmodel_notify_file_deleted(struct dirmodel *model, const char *filename)
{
	struct list *list = model->list;
	size_t index;

	bool found = dirmodel_get_index(model, filename, &index);
	if(found) {
		struct filedata *filedata = list_get_item(list, index);
		filedata_delete(filedata);
		list_remove(list, index);
		listmodel_notify_change(&model->listmodel, index, MODEL_REMOVE);
	}
}

int dirmodel_notify_file_added_or_changed(struct dirmodel *model, const char *filename)
{
	struct list *list = model->list;
	struct filedata *filedata;
	size_t index;

	if(model->filter_active && regexec(&model->filter, filename, 0, NULL, 0) != 0)
		return 0;

	int ret = filedata_new_from_file(&filedata, dirfd(model->dir), filename);
	if(ret != 0)
		return ret;

	bool found = list_find_item_or_insertpoint(list, filedata_listcompare_directory_filename, filedata, &index);
	if(found) {
		struct filedata *filedataold = list_get_item(list, index);
		list_set_item(list, index, filedata);
		filedata_delete(filedataold);
		listmodel_notify_change(&model->listmodel, index, MODEL_CHANGE);
	} else {
		if(!list_insert(list, index, filedata)) {
			filedata_delete(filedata);
			return ENOMEM;
		}
		listmodel_notify_change(&model->listmodel, index, MODEL_ADD);
	}

	return 0;
}

const char *dirmodel_getfilename(struct dirmodel *model, size_t index)
{
	struct list *list = model->list;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->filename;
}

bool dirmodel_isdir(struct dirmodel *model, size_t index)
{
	struct list *list = model->list;
	struct filedata *filedata = list_get_item(list, index);
	return S_ISDIR(filedata->stat.st_mode);
}

static bool internal_init(struct dirmodel *model, const char *path)
{
	DIR *dir;
	struct filedata *filedata;

	dir = opendir(path);
	if(dir == NULL)
		goto err_opendir;

	struct list *list = list_new(0);
	if(list == NULL)
		goto err_newlist;

	struct dirent *entry = readdir(dir);

	while(entry) {
		if(strcmp(entry->d_name, ".") != 0 &&
		   strcmp(entry->d_name, "..") != 0 &&
		   (model->filter_active ? regexec(&model->filter, entry->d_name, 0, NULL, 0) == 0 : true)) {
			int ret = filedata_new_from_file(&filedata, dirfd(dir), entry->d_name);

			if(ret == ENOMEM)
				goto err_readdir;

			if(ret != 0)
				continue;

			if(!list_append(list, filedata))
				goto err_readdir;
		}
		entry = readdir(dir);
	}
	model->dir = dir;
	model->list = list;

	list_sort(list, filedata_listcompare_directory_filename);

	return true;

err_readdir:
	filedata_delete(filedata);
	list_delete(list, (list_item_deallocator)filedata_delete);
err_newlist:
	closedir(dir);
err_opendir:
	return false;
}

static void internal_destroy(struct dirmodel *model)
{
	struct list *list = model->list;
	if(list == NULL)
		return;

	closedir(model->dir);

	list_delete(list, (list_item_deallocator)filedata_delete);
	model->list = NULL;
}

bool dirmodel_change_directory(struct dirmodel *model, const char *path)
{
	internal_destroy(model);
	if(!internal_init(model, path))
		return false;
	listmodel_notify_change(&model->listmodel, 0, MODEL_RELOAD);
	return true;
}

void dirmodel_init(struct dirmodel *model)
{
	listmodel_init(&model->listmodel);

	model->list = NULL;
	model->listmodel.count = dirmodel_count;
	model->listmodel.render = dirmodel_render;
	model->listmodel.setmark = dirmodel_setmark;
	model->listmodel.ismarked = dirmodel_ismarked;
	model->filter_active = false;
}

void dirmodel_destroy(struct dirmodel *model)
{
	internal_destroy(model);
	listmodel_destroy(&model->listmodel);
	if(model->filter_active)
		regfree(&model->filter);
}
