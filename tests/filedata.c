/* See LICENSE file for copyright and license details. */
#include <check.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wrapper/fstatat.h"
#include "../src/filedata.h"
#include "../src/util.h"
#include "tests.h"

#define PATH_TEMPLATE "/tmp/filedata.XXXXXX"

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
}

static void teardown(void)
{
	remove_directory_recursively(path);
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
	ck_assert(filedata != NULL);
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
	ck_assert_int_eq(symlinkat("foo", dir_fd, "bar"), 0);

	assert_oom(filedata_new_from_file(&filedata, dir_fd, "bar") == 0);
	ck_assert(filedata != NULL);
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

	ck_assert_int_eq(symlinkat("foo", dir_fd, "bar"), 0);

	assert_oom(filedata_new_from_file(&filedata, dir_fd, "bar") == 0);
	ck_assert(filedata != NULL);
	ck_assert(filedata->is_link == true);
	ck_assert(filedata->is_link_broken == true);
	ck_assert(filedata->stat.st_size == 3);
	ck_assert(filedata->is_marked == false);

	filedata_delete(filedata);
}
END_TEST

START_TEST(test_filedata_statfail)
{
	struct filedata *filedata;
	char buffer[FILEDATA_FORMAT_OUTPUT_BUFFER_SIZE];

	create_file(dir_fd, "foo", 1024);
	fstatat_seterrno(ENOTCONN);

	assert_oom(filedata_new_from_file(&filedata, dir_fd, "foo") == 0);
	filedata_format_output(filedata, buffer);
	ck_assert_str_eq(buffer, "?????????? ? ? ????""-??""-?? ??:??:??");

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
	tcase_add_test(tcase, test_filedata_statfail);
	suite_add_tcase(suite, tcase);

	return suite;
}
