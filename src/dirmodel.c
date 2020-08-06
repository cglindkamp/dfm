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

static int (*dirmodel_comparision_functions[])(const void *, const void *) = {
	[DIRMODEL_FILENAME] = filedata_listcompare_directory_filename,
	[DIRMODEL_FILENAME_DESCENDING] = filedata_listcompare_directory_filename_descending,
	[DIRMODEL_SIZE] = filedata_listcompare_directory_size_filename,
	[DIRMODEL_SIZE_DESCENDING] = filedata_listcompare_directory_size_filename_descending,
	[DIRMODEL_MTIME] = filedata_listcompare_directory_mtime_filename,
	[DIRMODEL_MTIME_DESCENDING] = filedata_listcompare_directory_mtime_filename_descending,
};

size_t dirmodel_count(struct listmodel *listmodel)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);
	struct list *list = model->list;
	if(list == NULL)
		return 0;
	return list_length(list);
}

static size_t dirmodel_render(struct listmodel *listmodel, wchar_t *buffer, size_t len, size_t width, size_t index)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);;
	struct list *list = model->sortedlist;
	struct filedata *filedata = list_get_item(list, index);

	return filedata_format_list_line(filedata, buffer, len, width);
}

static void dirmodel_setmark(struct listmodel *listmodel, size_t index, bool mark)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);;
	struct list *list = model->sortedlist;
	struct filedata *filedata = list_get_item(list, index);

	if(filedata->is_marked == mark)
		return;

	if(mark) {
		model->marked_stats.count++;
		model->marked_stats.size += filedata->stat.st_size;
	} else {
		model->marked_stats.count--;
		model->marked_stats.size -= filedata->stat.st_size;
	}
	filedata->is_marked = mark;
	listmodel_notify_change(listmodel, MODEL_CHANGE, index, index);
}

static bool dirmodel_ismarked(struct listmodel *listmodel, size_t index)
{
	struct dirmodel *model = container_of(listmodel, struct dirmodel, listmodel);;
	struct list *list = model->sortedlist;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->is_marked;
}

int dirmodel_getmarkedfilenames(struct dirmodel *model, const struct list **markedlist_out)
{
	struct list *list = model->sortedlist;
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

struct marked_stats dirmodel_getmarkedstats(struct dirmodel *model)
{
	return model->marked_stats;
}

bool dirmodel_get_internal_index(struct dirmodel *model, const char *filename, size_t *internal_index, size_t *index)
{
	struct filedata filedata;

	filedata.filename = filename;

	if(!list_find_item_or_insertpoint(model->list, filedata_listcompare_filename, &filedata, internal_index))
		return false;

	struct filedata *internal_filedata = list_get_item(model->list, *internal_index);
	if(!list_find_item_or_insertpoint(model->sortedlist, model->sort_compare, internal_filedata, index))
		return false;
	return true;
}

bool dirmodel_get_index(struct dirmodel *model, const char *filename, size_t *index)
{
	size_t internal_index;
	return dirmodel_get_internal_index(model, filename, &internal_index, index);
}

size_t dirmodel_regex_getnext(struct dirmodel *model, const char *regex, size_t start_index, int direction)
{
	struct list *list = model->sortedlist;
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
	struct list *list = model->sortedlist;
	regex_t cregex;

	int ret = regcomp(&cregex, regex, REG_EXTENDED | REG_ICASE | REG_NOSUB);
	if(ret != 0)
		return;

	size_t length = list_length(list);

	for(size_t i = 0; i < length; i++) {
		struct filedata *filedata = list_get_item(list, i);
		if(regexec(&cregex, filedata->filename, 0, NULL, 0) == 0 &&
		   filedata->is_marked != mark) {
			if(mark) {
				model->marked_stats.count++;
				model->marked_stats.size += filedata->stat.st_size;
			} else {
				model->marked_stats.count--;
				model->marked_stats.size -= filedata->stat.st_size;
			}
			filedata->is_marked = mark;
			listmodel_notify_change(&model->listmodel, MODEL_CHANGE, i, i);
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

void dirmodel_set_sort_mode(struct dirmodel *model, enum dirmodel_sort_mode mode)
{
	model->sort_compare = dirmodel_comparision_functions[mode];
}

void dirmodel_notify_file_deleted(struct dirmodel *model, const char *filename)
{
	struct list *list = model->list;
	size_t index, internal_index;

	bool found = dirmodel_get_internal_index(model, filename, &internal_index, &index);
	if(found) {
		struct filedata *filedata = list_get_item(list, internal_index);
		if(filedata->is_marked) {
			model->marked_stats.count--;
			model->marked_stats.size -= filedata->stat.st_size;
		}
		model->dirsize -= filedata->stat.st_size;
		filedata_delete(filedata);
		list_remove(list, internal_index);
		list_remove(model->sortedlist, index);
		listmodel_notify_change(&model->listmodel, MODEL_REMOVE, 0, index);
	}
}

static int dirmodel_update_file(struct dirmodel *model, struct filedata *newfiledata, size_t internal_index)
{
	struct filedata *oldfiledata = list_get_item(model->list, internal_index);
	size_t newindex, oldindex;

	newfiledata->is_marked = oldfiledata->is_marked;
	if(newfiledata->is_marked) {
		model->marked_stats.size -= oldfiledata->stat.st_size;
		model->marked_stats.size += newfiledata->stat.st_size;
	}
	model->dirsize -= oldfiledata->stat.st_size;
	model->dirsize += newfiledata->stat.st_size;

	list_find_item_or_insertpoint(model->sortedlist, model->sort_compare, oldfiledata, &oldindex);
	list_find_item_or_insertpoint(model->sortedlist, model->sort_compare, newfiledata, &newindex);

	if((newindex == oldindex) || (newindex == oldindex + 1)) {
		list_set_item(model->sortedlist, oldindex, newfiledata);
		listmodel_notify_change(&model->listmodel, MODEL_CHANGE, oldindex, oldindex);
	} else {
		if(oldindex < newindex)
			newindex--;
		list_move_item(model->sortedlist, oldindex, newindex);
		listmodel_notify_change(&model->listmodel, MODEL_CHANGE, newindex, oldindex);
	}
	list_set_item(model->list, internal_index, newfiledata);
	filedata_delete(oldfiledata);

	return 0;
}

static int dirmodel_add_file(struct dirmodel *model, struct filedata *filedata, size_t internal_index)
{
	if(!list_insert(model->list, internal_index, filedata)) {
		filedata_delete(filedata);
		return ENOMEM;
	}

	size_t index;
	list_find_item_or_insertpoint(model->sortedlist, model->sort_compare, filedata, &index);

	if(!list_insert(model->sortedlist, index, filedata)) {
		filedata_delete(filedata);
		list_remove(model->list, internal_index);
		return ENOMEM;
	}
	model->dirsize += filedata->stat.st_size;
	listmodel_notify_change(&model->listmodel, MODEL_ADD, index, 0);
	return 0;
}

static int dirmodel_notify_file_added_or_changed_real(struct dirmodel *model, const char *filename)
{
	struct filedata *filedata;
	size_t internal_index;

	if(model->filter_active && regexec(&model->filter, filename, 0, NULL, 0) != 0)
		return 0;

	int ret = filedata_new_from_file(&filedata, dirfd(model->dir), filename);
	if(ret != 0)
		return ret;

	bool found = list_find_item_or_insertpoint(model->list, filedata_listcompare_filename, filedata, &internal_index);
	if(found)
		return dirmodel_update_file(model, filedata, internal_index);
	else
		return dirmodel_add_file(model, filedata, internal_index);

	return 0;
}

static int listcompare_strcmp(const void *a, const void *b)
{
	char *string1 = *(char **)a;
	char *string2 = *(char **)b;

	return strcmp(string1, string2);
}

int dirmodel_notify_file_added_or_changed(struct dirmodel *model, const char *filename)
{
	size_t index;
	bool found = list_find_item_or_insertpoint(model->addchange_queue, listcompare_strcmp, filename, &index);
	if(!found) {
		char *filename_copy = strdup(filename);
		if(filename_copy == NULL)
			return ENOMEM;
		if(!list_insert(model->addchange_queue, index, filename_copy)) {
			free(filename_copy);
			return ENOMEM;
		}
	}
	return 0;
}

int dirmodel_notify_flush(struct dirmodel *model)
{
	int ret = 0;
	size_t length = list_length(model->addchange_queue);
	for(size_t i = 0; i < length; i++) {
		ret = dirmodel_notify_file_added_or_changed_real(model, list_get_item(model->addchange_queue, i));
		if(ret == ENOMEM)
			break;
	}
	for(size_t i = length; i > 0; i--) {
		free(list_get_item(model->addchange_queue, i - 1));
		list_remove(model->addchange_queue, i - 1);
	}
	return ret == ENOMEM ? ENOMEM : 0;
}

const char *dirmodel_getfilename(struct dirmodel *model, size_t index)
{
	struct list *list = model->sortedlist;
	struct filedata *filedata = list_get_item(list, index);
	return filedata->filename;
}

const struct filedata *dirmodel_getfiledata(struct dirmodel *model, size_t index)
{
	return list_get_item(model->sortedlist, index);
}

off_t dirmodel_getdirsize(struct dirmodel *model)
{
	return model->dirsize;
}

bool dirmodel_isdir(struct dirmodel *model, size_t index)
{
	struct list *list = model->sortedlist;
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

	model->addchange_queue = list_new(0);
	if(model->addchange_queue == NULL)
		goto err_new_addchange_queue;

	struct list *list = list_new(0);
	if(list == NULL)
		goto err_newlist;

	struct list *sortedlist = list_new(0);
	if(sortedlist == NULL)
		goto err_newsortedlist;

	struct dirent *entry = readdir(dir);

	model->dirsize = 0;
	while(entry) {
		if(strcmp(entry->d_name, ".") != 0 &&
		   strcmp(entry->d_name, "..") != 0 &&
		   (model->filter_active ? regexec(&model->filter, entry->d_name, 0, NULL, 0) == 0 : true)) {
			int ret = filedata_new_from_file(&filedata, dirfd(dir), entry->d_name);

			if(ret == ENOMEM)
				goto err_readdir;

			if(ret != 0)
				continue;

			if(!list_append(sortedlist, filedata))
				goto err_readdir;
			if(!list_append(list, filedata))
				goto err_readdir;
			model->dirsize += filedata->stat.st_size;
		}
		entry = readdir(dir);
	}
	model->dir = dir;
	model->list = list;
	model->sortedlist = sortedlist;

	list_sort(list, filedata_listcompare_filename);
	list_sort(sortedlist, model->sort_compare);

	model->marked_stats.count = 0;
	model->marked_stats.size = 0;

	return true;

err_readdir:
	filedata_delete(filedata);
	list_delete(sortedlist, NULL);
err_newsortedlist:
	list_delete(list, (list_item_deallocator)filedata_delete);
err_newlist:
	list_delete(model->addchange_queue, NULL);
err_new_addchange_queue:
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
	list_delete(model->sortedlist, NULL);
	list_delete(model->addchange_queue, free);
	model->list = NULL;
}

bool dirmodel_change_directory(struct dirmodel *model, const char *path)
{
	internal_destroy(model);
	if(!internal_init(model, path))
		return false;
	listmodel_notify_change(&model->listmodel, MODEL_RELOAD, 0, 0);
	return true;
}

void dirmodel_init(struct dirmodel *model)
{
	listmodel_init(&model->listmodel);

	model->list = NULL;
	model->sortedlist = NULL;
	model->listmodel.count = dirmodel_count;
	model->listmodel.render = dirmodel_render;
	model->listmodel.setmark = dirmodel_setmark;
	model->listmodel.ismarked = dirmodel_ismarked;
	model->filter_active = false;
	model->sort_compare = filedata_listcompare_directory_filename;
}

void dirmodel_destroy(struct dirmodel *model)
{
	internal_destroy(model);
	listmodel_destroy(&model->listmodel);
	if(model->filter_active)
		regfree(&model->filter);
}
