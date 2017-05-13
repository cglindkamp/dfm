#include <check.h>

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev.h>

#include "../src/dirmodel.h"

struct listmodel model;

static char *create_temp_directory()
{
	char path_template[] = "/tmp/dirmodel.XXXXXX";
	char *path = mkdtemp(path_template);

	return strdup(path);
}

static int remove_file(const char *path, const struct stat *sbuf, int type, struct FTW *ftwb)
{
	(void)sbuf;
	(void)type;
	(void)ftwb;

	remove(path);
	return 0;
}

static void remove_temp_directory(char *path)
{
	nftw(path, remove_file, 10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS);
	free(path);
}

static void create_file(int dir_fd, const char *filename, off_t size)
{
	int fd = openat(dir_fd, filename, O_CREAT|O_WRONLY, 0777);
	if(size > 0) {
		lseek(fd, size - 1, SEEK_SET);
		write(fd, "\0", 1);
	}
	close(fd);
}

static char *path;
static int dir_fd;

static void setup(void)
{
	path = create_temp_directory();
	dir_fd = open(path, O_RDONLY);
	dirmodel_init(&model);
}

static void teardown(void)
{
	dirmodel_free(&model);
	remove_temp_directory(path);
}

START_TEST(test_dirmodel_emptydirectory)
{
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 0);
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
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model, buf, 20, 20, 0), 20);
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
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), singlefiletesttable[_i].filename);
	ck_assert_uint_eq(listmodel_render(&model, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, singlefiletesttable[_i].rendered), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_singlelinktodirectory)
{
	symlinkat(".", dir_fd, "foo");

	wchar_t buf[21];
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo         -> <DIR>"), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_linktofile)
{
	create_file(dir_fd, "foofile", 0);
	symlinkat("foofile", dir_fd, "foo");

	wchar_t buf[21];
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 2);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo         ->    0 "), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_brokenlink)
{
	symlinkat("foofile", dir_fd, "foo");

	wchar_t buf[21];
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 1);
	ck_assert_str_eq(dirmodel_getfilename(&model, 0), "foo");
	ck_assert_uint_eq(listmodel_render(&model, buf, 20, 20, 0), 20);
	ck_assert_int_eq(wcscmp(buf, L"foo         ->    7 "), 0);
}
END_TEST

START_TEST(test_dirmodel_populateddirectory_order)
{
	mkdirat(dir_fd, "bard", 0x700);
	mkdirat(dir_fd, "food", 0x700);
	create_file(dir_fd, "barf", 0);
	create_file(dir_fd, "foof", 0);
	symlinkat("bard", dir_fd, "barl");
	symlinkat("foof", dir_fd, "fool");
	symlinkat("baz", dir_fd, "bazlbroken");

	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_count(&model), 7);
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
	ck_assert(dirmodel_change_directory(&model, path) == true);
	ck_assert_uint_eq(listmodel_render(&model, buf, renderspecialcasestesttable[_i].length, renderspecialcasestesttable[_i].width, 0), renderspecialcasestesttable[_i].neededlength);
	if(renderspecialcasestesttable[_i].rendered)
		ck_assert_int_eq(wcscmp(buf, renderspecialcasestesttable[_i].rendered), 0);
}
END_TEST

static size_t cb_index;
static enum model_change cb_change;
static unsigned int cb_count;

static void change_callback(size_t index, enum model_change change, void *data)
{
	(void)data;
	cb_count++;
	cb_index = index;
	cb_change = change;
}

START_TEST(test_dirmodel_reloadevent)
{
	cb_count = 0;
	cb_change = MODEL_CHANGE;

	listmodel_register_change_callback(&model, change_callback, NULL);

	ck_assert(dirmodel_change_directory(&model, path) == true);

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
	listmodel_register_change_callback(&model, change_callback, NULL);

	ck_assert(dirmodel_change_directory(&model, path) == true);
	struct ev_loop *loop = EV_DEFAULT;
	create_file(dir_fd, "2", 0);
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_index, 2);
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
	listmodel_register_change_callback(&model, change_callback, NULL);

	ck_assert(dirmodel_change_directory(&model, path) == true);
	struct ev_loop *loop = EV_DEFAULT;
	unlinkat(dir_fd, "3", 0);
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_index, 3);
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
	listmodel_register_change_callback(&model, change_callback, NULL);

	ck_assert(dirmodel_change_directory(&model, path) == true);
	struct ev_loop *loop = EV_DEFAULT;
	create_file(dir_fd, "1", 1);
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_index, 1);
	ck_assert_uint_eq(cb_change, MODEL_CHANGE);
}
END_TEST

START_TEST(test_dirmodel_addedfileremovedbeforeeventhandled)
{
	cb_count = 0;
	cb_change = MODEL_CHANGE;

	listmodel_register_change_callback(&model, change_callback, NULL);

	ck_assert(dirmodel_change_directory(&model, path) == true);
	struct ev_loop *loop = EV_DEFAULT;
	create_file(dir_fd, "foo", 0);
	unlinkat(dir_fd, "foo", 0);
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_change, MODEL_RELOAD);
}
END_TEST

START_TEST(test_dirmodel_markfiles)
{
	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "2", 0);
	create_file(dir_fd, "3", 0);
	create_file(dir_fd, "4", 0);

	ck_assert(dirmodel_change_directory(&model, path) == true);

	listmodel_setmark(&model, 1, true);
	listmodel_setmark(&model, 3, true);

	ck_assert(listmodel_ismarked(&model, 0) == false);
	ck_assert(listmodel_ismarked(&model, 1) == true);
	ck_assert(listmodel_ismarked(&model, 2) == false);
	ck_assert(listmodel_ismarked(&model, 3) == true);
	ck_assert(listmodel_ismarked(&model, 4) == false);
}
END_TEST

START_TEST(test_dirmodel_markfiles_changeevent)
{
	cb_count = 0;
	cb_index = 0;
	cb_change = MODEL_RELOAD;

	create_file(dir_fd, "0", 0);
	create_file(dir_fd, "1", 0);
	create_file(dir_fd, "2", 0);

	ck_assert(dirmodel_change_directory(&model, path) == true);
	listmodel_register_change_callback(&model, change_callback, NULL);

	listmodel_setmark(&model, 1, true);

	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_index, 1);
	ck_assert_uint_eq(cb_change, MODEL_CHANGE);
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
	tcase_add_test(tcase, test_dirmodel_addedfileremovedbeforeeventhandled);
	tcase_add_test(tcase, test_dirmodel_markfiles);
	tcase_add_test(tcase, test_dirmodel_markfiles_changeevent);
	suite_add_tcase(suite, tcase);

	return suite;
}
