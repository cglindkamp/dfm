/* See LICENSE file for copyright and license details. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <check.h>
#include <fcntl.h>
#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

#include "tests.h"
#include "wrapper/alloc.h"

void _nc_freeall();

Suite *list_suite(void);
Suite *dict_suite(void);
Suite *listmodel_suite(void);
Suite *listview_suite(void);
Suite *path_suite(void);
Suite *filedata_suite(void);
Suite *dirmodel_suite(void);
Suite *xdg_suite(void);
Suite *keymap_suite(void);
Suite *processmanager_suite(void);
Suite *commandline_suite(void);
Suite *clipboard_suite(void);

#define MAX_OOM_ITERATIONS 100
bool mode_oom = false;
int oom_pipe[2];

void reset_malloc() {
	alloc_set_allocations_until_fail(-1);
}

SRunner *create_runner()
{
	SRunner *suite_runner;
	suite_runner = srunner_create(list_suite());
	srunner_add_suite(suite_runner, dict_suite());
	srunner_add_suite(suite_runner, listmodel_suite());
	srunner_add_suite(suite_runner, listview_suite());
	srunner_add_suite(suite_runner, path_suite());
	srunner_add_suite(suite_runner, filedata_suite());
	srunner_add_suite(suite_runner, dirmodel_suite());
	srunner_add_suite(suite_runner, xdg_suite());
	srunner_add_suite(suite_runner, keymap_suite());
	srunner_add_suite(suite_runner, processmanager_suite());
	srunner_add_suite(suite_runner, commandline_suite());
	srunner_add_suite(suite_runner, clipboard_suite());

	return suite_runner;
}

int main(int argc, char *argv[])
{
	SRunner *suite_runner;
	int number_failed;

	FILE *dummyterm;

	setlocale(LC_ALL, "");
	if(strcmp(nl_langinfo(CODESET), "UTF-8") != 0 &&
	   setlocale(LC_ALL, "en_US.UTF-8") == NULL) {
		puts(
			"Current locale does not use UTF-8 encoding and en_US.UTF-8 as fallback was\n"
			"also not available. UTF-8 tests would fail.\n"
			"Please set locale to an available one with UTF-8 encoding and rerun tests.");
		return EXIT_FAILURE;
	}

	atexit(reset_malloc);

	dummyterm = fopen("/dev/null", "rw");
	newterm(NULL, dummyterm, dummyterm);

	if(argc == 2 && strcmp(argv[1], "oom") == 0) {
		if(pipe2(oom_pipe, O_NONBLOCK)) {
			number_failed = 1;
			perror("Failed to create pipe");
			goto out;
		}

		mode_oom = true;
		puts("OOM test pass: ");

		for(int i = 0; i < MAX_OOM_ITERATIONS; i++) {
			char buf;
			printf("%d ", i + 1);
			alloc_set_allocations_until_fail(i);

			suite_runner = create_runner();
			srunner_set_fork_status(suite_runner, CK_FORK);
			srunner_run_all(suite_runner, CK_SILENT);

			number_failed = srunner_ntests_failed(suite_runner);
			if(number_failed > 0) {
				puts("failed");
				srunner_print(suite_runner, CK_NORMAL);
				srunner_free(suite_runner);
				break;
			}
			srunner_free(suite_runner);

			/* if no tests signaled an OOM situation, we are finished */
			if(read(oom_pipe[0], &buf, 1) != 1) {
				puts("passed");
				break;
			}

			/* drain the pipe */
			while(read(oom_pipe[0], &buf, 1) == 1);
		}

		close(oom_pipe[0]);
		close(oom_pipe[1]);
	} else {
		suite_runner = create_runner();
		srunner_run_all(suite_runner, CK_NORMAL);
		number_failed = srunner_ntests_failed(suite_runner);
		srunner_free(suite_runner);
	}

out:
	alloc_set_allocations_until_fail(-1);
	fclose(dummyterm);
	_nc_freeall();
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
