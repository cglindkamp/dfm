/* See LICENSE file for copyright and license details. */
#include <check.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../src/processmanager.h"
#include "../src/util.h"
#include "tests.h"

#define PATH_TEMPLATE "/tmp/processmanager.XXXXXX"

static struct {
	const char *executable_path;
	char *executable_name;
	int exit_status;
} spawn_testtable[] = {
	{ "/bin/true", "true", 0 },
	{ "/bin/false", "false", EXIT_FAILURE },
	{ "/foo/bar", "bar", EXIT_FAILURE },
};

START_TEST(test_processmanager_spawn)
{
	struct processmanager pm;
	char * const args[] = {
		spawn_testtable[_i].executable_name,
		NULL
	};
	pid_t pid;
	int status;

	processmanager_init(&pm);

	int ret = processmanager_spawn(&pm, spawn_testtable[_i].executable_path, args, NULL, true, &pid);
	ck_assert_int_eq(ret, 0);
	ck_assert(processmanager_waitpid(&pm, pid, &status) == pid);
	ck_assert(WIFEXITED(status));
	ck_assert(WEXITSTATUS(status) == spawn_testtable[_i].exit_status);

	processmanager_destroy(&pm);
}
END_TEST

START_TEST(test_processmanager_tmpdircleanup)
{
	struct processmanager pm;
	char * const args[] = {
		"true",
		NULL
	};
	pid_t pid;
	int status;
	struct stat buf;
	char path[] = PATH_TEMPLATE;

	ck_assert(mkdtemp(path) != NULL);

	processmanager_init(&pm);

	int ret = processmanager_spawn(&pm, "/bin/true", args, path, true, &pid);
	assert_oom_cleanup(ret != ENOMEM, processmanager_destroy(&pm), remove_directory_recursively(path));
	ck_assert_int_eq(ret, 0);

	ck_assert_int_eq(stat(path, &buf), 0);
	ck_assert(processmanager_waitpid(&pm, pid, &status) == pid);
	ck_assert_int_eq(stat(path, &buf), -1);

	processmanager_destroy(&pm);
}
END_TEST

Suite *processmanager_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("ProcessManager");

	tcase = tcase_create("Core");
	tcase_add_loop_test(tcase, test_processmanager_spawn, 0, sizeof(spawn_testtable)/sizeof(spawn_testtable[0]));
	tcase_add_test(tcase, test_processmanager_tmpdircleanup);
	suite_add_tcase(suite, tcase);

	return suite;
}
