/* See LICENSE file for copyright and license details. */
#include <check.h>

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/filedata.h"
#include "tests.h"

#define PATH_TEMPLATE "/tmp/filedata.XXXXXX"

static char path[] = PATH_TEMPLATE;
static int dir_fd;

static void create_temp_directory()
{
	strcpy(path, PATH_TEMPLATE);
	ck_assert_ptr_nonnull(mkdtemp(path));
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

static void setup(void)
{
	create_temp_directory();
	dir_fd = open(path, O_RDONLY);
}

static void teardown(void)
{
	remove_temp_directory(path);
}

START_TEST(test_filedata_filenonexistent)
{
	struct filedata *filedata;

	ck_assert_int_eq(filedata_new_from_file(&filedata, dir_fd, "foo"), ENOENT);
}
END_TEST

START_TEST(test_filedata_regularfile)
{
	struct filedata *filedata;

	create_file(dir_fd, "foo", 1024);

	assert_oom(filedata_new_from_file(&filedata, dir_fd, "foo") == 0);
	ck_assert_ptr_nonnull(filedata);
	ck_assert(filedata->is_link == false);
	ck_assert(filedata->stat.st_size == 1024);
	ck_assert(S_ISREG(filedata->stat.st_mode));
	ck_assert(filedata->is_marked == false);

	filedata_delete(filedata);
}
END_TEST

START_TEST(test_filedata_link)
{
	struct filedata *filedata;

	create_file(dir_fd, "foo", 2048);
	symlinkat("foo", dir_fd, "bar");

	assert_oom(filedata_new_from_file(&filedata, dir_fd, "bar") == 0);
	ck_assert_ptr_nonnull(filedata);
	ck_assert(filedata->is_link == true);
	ck_assert(filedata->is_link_broken == false);
	ck_assert(filedata->stat.st_size == 2048);
	ck_assert(S_ISREG(filedata->stat.st_mode));
	ck_assert(filedata->is_marked == false);

	filedata_delete(filedata);
}
END_TEST

START_TEST(test_filedata_linkbroken)
{
	struct filedata *filedata;

	symlinkat("foo", dir_fd, "bar");

	assert_oom(filedata_new_from_file(&filedata, dir_fd, "bar") == 0);
	ck_assert_ptr_nonnull(filedata);
	ck_assert(filedata->is_link == true);
	ck_assert(filedata->is_link_broken == true);
	ck_assert(filedata->stat.st_size == 3);
	ck_assert(filedata->is_marked == false);

	filedata_delete(filedata);
}
END_TEST

Suite *filedata_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Filedata");

	tcase = tcase_create("Core");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_filedata_filenonexistent);
	tcase_add_test(tcase, test_filedata_regularfile);
	tcase_add_test(tcase, test_filedata_link);
	tcase_add_test(tcase, test_filedata_linkbroken);
	suite_add_tcase(suite, tcase);

	return suite;
}
