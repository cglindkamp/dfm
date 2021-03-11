/* See LICENSE file for copyright and license details. */
#include <check.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wrapper/fstatat.h"
#include "../src/dirmodel.h"
#include "../src/list.h"
#include "../src/util.h"
#include "tests.h"

#define PATH_TEMPLATE "/tmp/dirmodel.XXXXXX"

static struct dirmodel model;
static char path[] = PATH_TEMPLATE;
static int dir_fd;

static void create_temp_directory()
{
	strcpy(path, PATH_TEMPLATE);
	ck_assert(mkdtemp(path) != NULL);
}

static void create_file(int dir_fd, const char *filename, off_t size)
{
	int fd = openat(dir_fd, filename, O_CREAT|O_WRONLY, 0777);
	if(size > 0) {
		lseek(fd, size - 1, SEEK_SET);
		ck_assert_int_eq(write(fd, "\0", 1), 1);
	}
	close(fd);
}

static void setup(void)
{
	create_temp_directory();
	dir_fd = open(path, O_RDONLY);
	dirmodel_init(&model);
}

static void teardown(void)
{
	fstatat_seterrno(0);
	dirmodel_destroy(&model);
	remove_directory_recursively(path);
}

START_TEST(test_dirmodel_emptydirectory)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 0);
}
END_TEST

START_TEST(test_dirmodel_unreadabledirectory)
{
	chmod(path, S_ISVTX|S_IWUSR);

	ck_assert(dirmodel_change_directory(&model, path) == false);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_singledirectory)
{
	mkdirat(dir_fd, "foo", 0x700);

	wchar_t buf[21];
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model.listmodel, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo            <DIR>"), 0);
}
END_TEST

static struct {
	const char *filename;
	const wchar_t *rendered;
	off_t size;
} singlefiletesttable[] = {
	{ "foo", L"foo            1024K", 1 * 1024 * 1024 },
	{ "foo", L"foo             4.0M", 4 * 1024 * 1024 },
	{ "foo", L"foo            10.5M", 10 * 1024 * 1024 + 512 * 1024},
	{ "foo", L"foo             400M", 400 * 1024 * 1024 },
};

START_TEST(test_dirmodel_populateddirectory_singlefile)
{
	create_file(dir_fd, singlefiletesttable[_i].filename, singlefiletesttable[_i].size);

	wchar_t buf[21];
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), singlefiletesttable[_i].filename);
	ck_assert_uint_eq(listmodel_render(&model.listmodel, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, singlefiletesttable[_i].rendered), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_singlelinktodirectory)
{
	ck_assert_int_eq(symlinkat(".", dir_fd, "foo"), 0);

	wchar_t buf[21];
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model.listmodel, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo         -> <DIR>"), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_linktofile)
{
	create_file(dir_fd, "foofile", 0);
	ck_assert_int_eq(symlinkat("foofile", dir_fd, "foo"), 0);

	wchar_t buf[21];
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 2);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model.listmodel, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo         ->    0 "), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_brokenlink)
{
	ck_assert_int_eq(symlinkat("foofile", dir_fd, "foo"), 0);

	wchar_t buf[21];
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model.listmodel, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo         -X    7 "), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_order)
{
	mkdirat(dir_fd, "bard", 0x700);
	mkdirat(dir_fd, "food", 0x700);
	create_file(dir_fd, "barf", 0);
	create_file(dir_fd, "foof", 0);
	ck_assert_int_eq(symlinkat("bard", dir_fd, "barl"), 0);
	ck_assert_int_eq(symlinkat("foof", dir_fd, "fool"), 0);
	ck_assert_int_eq(symlinkat("baz", dir_fd, "bazlbroken"), 0);

	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model.listmodel), 7);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "bard");
	ck_assert_str_eq(dirmodel_getfilename(&model, 1), "barl");
	ck_assert_str_eq(dirmodel_getfilename(&model, 2), "food");
	ck_assert_str_eq(dirmodel_getfilename(&model, 3), "barf");
	ck_assert_str_eq(dirmodel_getfilename(&model, 4), "bazlbroken");
	ck_assert_str_eq(dirmodel_getfilename(&model, 5), "foof");
	ck_assert_str_eq(dirmodel_getfilename(&model, 6), "fool");
}
END_TEST

static struct {
	const char *name;
	const wchar_t *rendered;
	size_t length;
	size_t width;
	size_t neededlength;
} renderspecialcasestesttable[] = {
	{ "foobar", L"foob <DIR>", 10, 10, 10 },
	{ "foobar", L"DIR>", 4, 4, 4 },
	{ "foo\nbar", L"foobar <DIR>", 12, 12, 12 },
	{ u8"fo\u0308o\u0308bar", L"fo\u0308o\u0308bar         <DIR>", 22, 20, 22 },
	{ u8"fo\u0308o\u0308bar", NULL, 8, 8, 9 },
	{ u8"fo\u0308o\u0308bar", L"fo\u0308 <DIR>", 9, 8, 9 },
	{ "\xff", L"???  <DIR>", 10, 10, 10 },
	{ "\xff", L"?? <DIR>", 8, 8, 8 },
};

START_TEST(test_dirmodel_render_specialcases)
{

	mkdirat(dir_fd, renderspecialcasestesttable[_i].name, 0x700);

	wchar_t buf[renderspecialcasestesttable[_i].length + 1];
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_render(&model.listmodel, buf, renderspecialcasestesttable[_i].length, renderspecialcasestesttable[_i].width, 0), renderspecialcasestesttable[_i].neededlength);
	if(renderspecialcasestesttable[_i].rendered)
		ck_assert_int_eq(wcscmp(buf, renderspecialcasestesttable[_i].rendered), 0);
}
END_TEST

static size_t cb_newindex;
static size_t cb_oldindex;
static enum model_change cb_change;
static unsigned int cb_count;

static void change_callback(enum model_change change, size_t newindex, size_t oldindex, void *data)
{
	(void)data;
	cb_count++;
	cb_newindex = newindex;
	cb_oldindex = oldindex;
	cb_change = change;
}

START_TEST(test_dirmodel_reloadevent)
{
	cb_count = 0;
	cb_change = MODEL_CHANGE;

	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_change, MODEL_RELOAD);
}
END_TEST

START_TEST(test_dirmodel_addedfileevent)
{
	cb_count = 0;
	cb_change = MODEL_RELOAD;

	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "3", 0);
	create_file(dir_fd, "4", 0);
	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	assert_oom(dirmodel_change_directory(&model, path) == true);
	create_file(dir_fd, "2", 0);

	assert_oom(dirmodel_notify_file_added_or_changed(&model, "2") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_newindex, 2);
	ck_assert_uint_eq(cb_change, MODEL_ADD);
}
END_TEST

START_TEST(test_dirmodel_removedfileevent)
{
	cb_count = 0;
	cb_change = MODEL_RELOAD;

	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "2", 0);
	create_file(dir_fd, "3", 0);
	create_file(dir_fd, "4", 0);
	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	assert_oom(dirmodel_change_directory(&model, path) == true);
	dirmodel_notify_file_deleted(&model, "3");

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_oldindex, 3);
	ck_assert_uint_eq(cb_change, MODEL_REMOVE);
}
END_TEST

START_TEST(test_dirmodel_changedfileevent)
{
	cb_count = 0;
	cb_change = MODEL_RELOAD;

	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "2", 0);
	create_file(dir_fd, "3", 0);
	create_file(dir_fd, "4", 0);
	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	assert_oom(dirmodel_change_directory(&model, path) == true);

	assert_oom(dirmodel_notify_file_added_or_changed(&model, "1") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_newindex, 1);
	ck_assert_uint_eq(cb_oldindex, 1);
	ck_assert_uint_eq(cb_change, MODEL_CHANGE);
}
END_TEST

START_TEST(test_dirmodel_changedfileevent_newposition)
{
	cb_count = 0;
	cb_change = MODEL_RELOAD;

	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "2", 0);
	create_file(dir_fd, "3", 0);
	create_file(dir_fd, "4", 0);
	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	assert_oom(dirmodel_change_directory(&model, path) == true);

	unlinkat(dir_fd, "1", 0);
	mkdirat(dir_fd, "1", 0x700);
	assert_oom(dirmodel_notify_file_added_or_changed(&model, "1") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_newindex, 0);
	ck_assert_uint_eq(cb_oldindex, 1);
	ck_assert_uint_eq(cb_change, MODEL_CHANGE);
}
END_TEST

START_TEST(test_dirmodel_addedfileremovedbeforeeventhandled)
{
	cb_count = 0;
	cb_change = MODEL_CHANGE;

	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	assert_oom(dirmodel_change_directory(&model, path) == true);

	/* we simulate this situation by notifying dirmodel without creating the file */
	assert_oom(dirmodel_notify_file_added_or_changed(&model, "foo") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_change, MODEL_RELOAD);
}
END_TEST

/* regression test for endless loop in internal_init */
START_TEST(test_dirmodel_statfail)
{
	create_file(dir_fd, "foo", 0);
	fstatat_seterrno(ENOENT);
	assert_oom(dirmodel_change_directory(&model, path) == true);
}
END_TEST

START_TEST(test_dirmodel_dirsize_files)
{
	create_file(dir_fd, "foo", 10);
	create_file(dir_fd, "bar", 25);
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert_uint_eq(dirmodel_getdirsize(&model), 35);
}
END_TEST

START_TEST(test_dirmodel_dirsize_file_and_symlink)
{
	const off_t filesize = 10;

	create_file(dir_fd, "foo", filesize);
	ck_assert_int_eq(symlinkat("foo", dir_fd, "bar"), 0);
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert_uint_eq(dirmodel_getdirsize(&model), filesize + strlen("bar"));

	dirmodel_regex_setmark(&model, "bar", 1);
	struct marked_stats stats = dirmodel_getmarkedstats(&model);
	ck_assert_uint_eq(stats.count, 1);
	ck_assert_uint_eq(stats.size, strlen("bar"));

	dirmodel_regex_setmark(&model, "bar", 0);
	stats = dirmodel_getmarkedstats(&model);
	ck_assert_uint_eq(stats.count, 0);
	ck_assert_uint_eq(stats.size, 0);

	listmodel_setmark(&(model.listmodel), 0, 1);
	stats = dirmodel_getmarkedstats(&model);
	ck_assert_uint_eq(stats.count, 1);
	ck_assert_uint_eq(stats.size, strlen("bar"));

	listmodel_setmark(&model.listmodel, 0, 0);
	stats = dirmodel_getmarkedstats(&model);
	ck_assert_uint_eq(stats.count, 0);
	ck_assert_uint_eq(stats.size, 0);
}
END_TEST

START_TEST(test_dirmodel_dirsize_file_and_symlink_added)
{
	const off_t filesize = 10;

	create_file(dir_fd, "foo", filesize);
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(dirmodel_getdirsize(&model), filesize);

	ck_assert_int_eq(symlinkat("foo", dir_fd, "bar"), 0);
	assert_oom(dirmodel_notify_file_added_or_changed(&model, "bar") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(dirmodel_getdirsize(&model), filesize + strlen("bar"));
}
END_TEST

START_TEST(test_dirmodel_dirsize_file_and_symlink_removed)
{
	const off_t filesize = 10;

	create_file(dir_fd, "foo", filesize);
	ck_assert_int_eq(symlinkat("foo", dir_fd, "bar"), 0);
	assert_oom(dirmodel_change_directory(&model, path) == true);

	dirmodel_regex_setmark(&model, "bar", 1);

	dirmodel_notify_file_deleted(&model, "bar");
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(dirmodel_getdirsize(&model), filesize);

	struct marked_stats stats = dirmodel_getmarkedstats(&model);
	ck_assert_uint_eq(stats.count, 0);
	ck_assert_uint_eq(stats.size, 0);
}
END_TEST

START_TEST(test_dirmodel_dirsize_file_and_symlink_changed)
{
	const off_t filesize = 10;

	create_file(dir_fd, "foo", filesize);
	ck_assert_int_eq(symlinkat("foo", dir_fd, "bar"), 0);
	assert_oom(dirmodel_change_directory(&model, path) == true);

	dirmodel_regex_setmark(&model, "bar", 1);

	create_file(dir_fd, "foo", filesize * 2);
	assert_oom(dirmodel_notify_file_added_or_changed(&model, "foo") != ENOMEM);
	assert_oom(dirmodel_notify_file_added_or_changed(&model, "bar") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(dirmodel_getdirsize(&model), filesize * 2 + strlen("bar"));

	struct marked_stats stats = dirmodel_getmarkedstats(&model);
	ck_assert_uint_eq(stats.count, 1);
	ck_assert_uint_eq(stats.size, strlen("bar"));
}
END_TEST

static void setup_markfiles(void)
{
	setup();
	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "2", 0);
	create_file(dir_fd, "3", 0);
	create_file(dir_fd, "4", 0);
}

START_TEST(test_dirmodel_markfiles)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	listmodel_setmark(&model.listmodel, 1, true);
	listmodel_setmark(&model.listmodel, 3, true);

	ck_assert(listmodel_ismarked(&model.listmodel, 0) == false);
	ck_assert(listmodel_ismarked(&model.listmodel, 1) == true);
	ck_assert(listmodel_ismarked(&model.listmodel, 2) == false);
	ck_assert(listmodel_ismarked(&model.listmodel, 3) == true);
	ck_assert(listmodel_ismarked(&model.listmodel, 4) == false);
}
END_TEST

START_TEST(test_dirmodel_markfiles_changeevent)
{
	cb_count = 0;
	cb_newindex = 0;
	cb_change = MODEL_RELOAD;

	assert_oom(dirmodel_change_directory(&model, path) == true);
	assert_oom(listmodel_register_change_callback(&model.listmodel, change_callback, NULL) == true);

	listmodel_setmark(&model.listmodel, 1, true);

	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_newindex, 1);
	ck_assert_uint_eq(cb_change, MODEL_CHANGE);
}
END_TEST

START_TEST(test_dirmodel_markfiles_getfilenames)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	listmodel_setmark(&model.listmodel, 1, true);
	listmodel_setmark(&model.listmodel, 3, true);
	listmodel_setmark(&model.listmodel, 4, true);

	const struct list *list;
	int ret = dirmodel_getmarkedfilenames(&model, &list);
	assert_oom(ret != ENOMEM);
	ck_assert(ret == 0);
	ck_assert_uint_eq(list_length(list), 3);
	ck_assert_str_eq(list_get_item(list, 0), "1");
	ck_assert_str_eq(list_get_item(list, 1), "3");
	ck_assert_str_eq(list_get_item(list, 2), "4");

	list_delete(list, free);
}
END_TEST

START_TEST(test_dirmodel_markfiles_getfilenames_nomarkedfiles)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	const struct list *list;
	int ret = dirmodel_getmarkedfilenames(&model, &list);
	assert_oom(ret != ENOMEM);
	ck_assert(ret == ENOENT);
}
END_TEST

static void setup_regex(void)
{
	setup();
	create_file(dir_fd, "bar", 0);
	create_file(dir_fd, "barfoo", 0);
	create_file(dir_fd, "barfop", 0);
	create_file(dir_fd, "bazfop", 0);
	create_file(dir_fd, "foo", 0);
}

START_TEST(test_dirmodel_regexsearch_notfoundforward)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert(dirmodel_regex_getnext(&model, "frob", 0, 1) == 0);
	ck_assert(dirmodel_regex_getnext(&model, "frob", 2, 1) == 2);
	ck_assert(dirmodel_regex_getnext(&model, "frob", SIZE_MAX, 1) == SIZE_MAX);
}
END_TEST

START_TEST(test_dirmodel_regexsearch_notfoundbackward)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);
	ck_assert(dirmodel_regex_getnext(&model, "frob", 0, -1) == 0);
	ck_assert(dirmodel_regex_getnext(&model, "frob", 3, -1) == 3);
}
END_TEST

START_TEST(test_dirmodel_regexsearch_foundforward)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert(dirmodel_regex_getnext(&model, "foo", 2, -1) == 1);
	ck_assert(dirmodel_regex_getnext(&model, "bar", 4, -1) == 2);
	ck_assert(dirmodel_regex_getnext(&model, "ba.", 4, -1) == 3);
	ck_assert(dirmodel_regex_getnext(&model, "foo", 0, -1) == 0);
}
END_TEST

START_TEST(test_dirmodel_regexsearch_foundbackward)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert(dirmodel_regex_getnext(&model, "foo", 0, 1) == 1);
	ck_assert(dirmodel_regex_getnext(&model, "foo", 2, 1) == 4);
	ck_assert(dirmodel_regex_getnext(&model, "fo.", 2, 1) == 3);
}
END_TEST

START_TEST(test_dirmodel_regexmark)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	dirmodel_regex_setmark(&model, ".fo", true);

	const struct list *list;
	int ret = dirmodel_getmarkedfilenames(&model, &list);
	assert_oom(ret != ENOMEM);
	ck_assert(ret == 0);
	ck_assert_uint_eq(list_length(list), 3);
	ck_assert_str_eq(list_get_item(list, 0), "barfoo");
	ck_assert_str_eq(list_get_item(list, 1), "barfop");
	ck_assert_str_eq(list_get_item(list, 2), "bazfop");

	list_delete(list, free);
}
END_TEST

START_TEST(test_dirmodel_regexunmark)
{
	assert_oom(dirmodel_change_directory(&model, path) == true);

	dirmodel_regex_setmark(&model, ".*", true);
	dirmodel_regex_setmark(&model, "foo", false);

	const struct list *list;
	int ret = dirmodel_getmarkedfilenames(&model, &list);
	assert_oom(ret != ENOMEM);
	ck_assert(ret == 0);
	ck_assert_uint_eq(list_length(list), 3);
	ck_assert_str_eq(list_get_item(list, 0), "bar");
	ck_assert_str_eq(list_get_item(list, 1), "barfop");
	ck_assert_str_eq(list_get_item(list, 2), "bazfop");

	list_delete(list, free);
}
END_TEST

START_TEST(test_dirmodel_setfilter_null)
{
	dirmodel_setfilter(&model, NULL);
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert_uint_eq(listmodel_count(&model.listmodel), 5);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "bar");
	ck_assert_str_eq(dirmodel_getfilename(&model, 1), "barfoo");
	ck_assert_str_eq(dirmodel_getfilename(&model, 2), "barfop");
	ck_assert_str_eq(dirmodel_getfilename(&model, 3), "bazfop");
	ck_assert_str_eq(dirmodel_getfilename(&model, 4), "foo");
}
END_TEST

START_TEST(test_dirmodel_setfilter_filter)
{
	dirmodel_setfilter(&model, ".fo");
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert_uint_eq(listmodel_count(&model.listmodel), 3);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "barfoo");
	ck_assert_str_eq(dirmodel_getfilename(&model, 1), "barfop");
	ck_assert_str_eq(dirmodel_getfilename(&model, 2), "bazfop");
}
END_TEST

START_TEST(test_dirmodel_setfilter_filteraddedfile)
{
	dirmodel_setfilter(&model, "^[^.]");
	assert_oom(dirmodel_change_directory(&model, path) == true);

	ck_assert_uint_eq(listmodel_count(&model.listmodel), 5);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "bar");
	ck_assert_str_eq(dirmodel_getfilename(&model, 1), "barfoo");
	ck_assert_str_eq(dirmodel_getfilename(&model, 2), "barfop");
	ck_assert_str_eq(dirmodel_getfilename(&model, 3), "bazfop");
	ck_assert_str_eq(dirmodel_getfilename(&model, 4), "foo");

	create_file(dir_fd, ".hiddenfile", 0);
	assert_oom(dirmodel_notify_file_added_or_changed(&model, ".hiddenfile") != ENOMEM);
	assert_oom(dirmodel_notify_flush(&model) != ENOMEM);

	ck_assert_uint_eq(listmodel_count(&model.listmodel), 5);
}
END_TEST

Suite *dirmodel_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Dirmodel");

	tcase = tcase_create("Core");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_dirmodel_emptydirectory);
	tcase_add_test(tcase, test_dirmodel_unreadabledirectory);
	tcase_add_test(tcase, test_dirmodel_populateddirectory_singledirectory);
	tcase_add_loop_test(tcase, test_dirmodel_populateddirectory_singlefile, 0, sizeof(singlefiletesttable)/sizeof(singlefiletesttable[0]));
	tcase_add_test(tcase, test_dirmodel_populateddirectory_singlelinktodirectory);
	tcase_add_test(tcase, test_dirmodel_populateddirectory_linktofile);
	tcase_add_test(tcase, test_dirmodel_populateddirectory_brokenlink);
	tcase_add_test(tcase, test_dirmodel_populateddirectory_order);
	tcase_add_loop_test(tcase, test_dirmodel_render_specialcases, 0, sizeof(renderspecialcasestesttable)/sizeof(renderspecialcasestesttable[0]));
	tcase_add_test(tcase, test_dirmodel_reloadevent);
	tcase_add_test(tcase, test_dirmodel_addedfileevent);
	tcase_add_test(tcase, test_dirmodel_removedfileevent);
	tcase_add_test(tcase, test_dirmodel_changedfileevent);
	tcase_add_test(tcase, test_dirmodel_changedfileevent_newposition);
	tcase_add_test(tcase, test_dirmodel_addedfileremovedbeforeeventhandled);
	tcase_add_test(tcase, test_dirmodel_statfail);
	tcase_add_test(tcase, test_dirmodel_dirsize_files);
	tcase_add_test(tcase, test_dirmodel_dirsize_file_and_symlink);
	tcase_add_test(tcase, test_dirmodel_dirsize_file_and_symlink_added);
	tcase_add_test(tcase, test_dirmodel_dirsize_file_and_symlink_removed);
	tcase_add_test(tcase, test_dirmodel_dirsize_file_and_symlink_changed);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("Mark Files");
	tcase_add_checked_fixture(tcase, setup_markfiles, teardown);
	tcase_add_test(tcase, test_dirmodel_markfiles);
	tcase_add_test(tcase, test_dirmodel_markfiles_changeevent);
	tcase_add_test(tcase, test_dirmodel_markfiles_getfilenames);
	tcase_add_test(tcase, test_dirmodel_markfiles_getfilenames_nomarkedfiles);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("Regex");
	tcase_add_checked_fixture(tcase, setup_regex, teardown);
	tcase_add_test(tcase, test_dirmodel_regexsearch_notfoundforward);
	tcase_add_test(tcase, test_dirmodel_regexsearch_notfoundbackward);
	tcase_add_test(tcase, test_dirmodel_regexsearch_foundforward);
	tcase_add_test(tcase, test_dirmodel_regexsearch_foundbackward);
	tcase_add_test(tcase, test_dirmodel_regexmark);
	tcase_add_test(tcase, test_dirmodel_regexunmark);
	tcase_add_test(tcase, test_dirmodel_setfilter_null);
	tcase_add_test(tcase, test_dirmodel_setfilter_filter);
	tcase_add_test(tcase, test_dirmodel_setfilter_filteraddedfile);
	suite_add_tcase(suite, tcase);

	return suite;
}
