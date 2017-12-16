/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/clipboard.h"
#include "../src/util.h"
#include "tests.h"

char *__real_strdup(const char *s);

#define PATH_TEMPLATE "/tmp/clipboard.XXXXXX"

static bool is_directory_empty(int dir_fd) {
	struct dirent *entry;
	DIR *dir = fdopendir(dir_fd);
	unsigned int count = 0;

	if(dir == NULL)
		return false;

	while((entry = readdir(dir)) != NULL) {
		if(++count > 2)
			break;
	}
	closedir(dir);

	if(count <= 2)
		return true;
	else
		return false;
}

static bool does_file_contain(int dir_fd, const char *filename, const char *contents, off_t size)
{
	int fd = openat(dir_fd, filename, O_RDONLY);
	off_t filesize = lseek(fd, 0, SEEK_END);

	if(filesize != size) {
		close(fd);
		return false;
	}

	lseek(fd, 0, SEEK_SET);
	char buffer[size];
	if(read(fd, buffer, size) != size)
		return false;

	close(fd);

	if(memcmp(buffer, contents, size) == 0)
		return true;

	return false;
}

START_TEST(test_clipboard_empty)
{
	struct clipboard clipboard;
	static char path[] = PATH_TEMPLATE;
	ck_assert(mkdtemp(path) != NULL);
	int dir_fd = open(path, O_RDONLY);

	clipboard_init(&clipboard);

	ck_assert(clipboard_dump_contents_to_directory(&clipboard, dir_fd) == true);
	ck_assert(is_directory_empty(dir_fd) == true);

	close(dir_fd);
	remove_directory_recursively(path);
	clipboard_destroy(&clipboard);
}
END_TEST

START_TEST(test_clipboard_nonempty)
{
	char *cpath = __real_strdup("foo");
	list_t *clist = list_new(0);

	assert_oom(clist != NULL);
	assert_oom(list_append(clist, __real_strdup("1")));
	assert_oom(list_append(clist, __real_strdup("2")));

	struct clipboard clipboard;
	static char path[] = PATH_TEMPLATE;
	ck_assert(mkdtemp(path) != NULL);
	int dir_fd = open(path, O_RDONLY);

	clipboard_init(&clipboard);

	clipboard_set_contents(&clipboard, cpath, clist);
	ck_assert(clipboard_dump_contents_to_directory(&clipboard, dir_fd) == true);

	ck_assert(does_file_contain(dir_fd, "clipboard_path", "foo", 3) == true);
	ck_assert(does_file_contain(dir_fd, "clipboard_list", "1\0""2\0", 4) == true);

	close(dir_fd);
	remove_directory_recursively(path);
	clipboard_destroy(&clipboard);
}
END_TEST

Suite *clipboard_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Clipboard");

	tcase = tcase_create("Core");
	tcase_add_test(tcase, test_clipboard_empty);
	tcase_add_test(tcase, test_clipboard_nonempty);
	suite_add_tcase(suite, tcase);

	return suite;
}
