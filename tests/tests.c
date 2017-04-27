#include <check.h>
#include <stdlib.h>
#include <ncurses.h>

Suite *list_suite(void);
Suite *dict_suite(void);
Suite *listmodel_suite(void);
Suite *listview_suite(void);
Suite *path_suite(void);

int main(void)
{
	SRunner *suite_runner;
	int number_failed;

	FILE *dummyterm;
	SCREEN *dummyscreen;

	dummyterm = fopen("/dev/null", "rw");
	dummyscreen = newterm(NULL, dummyterm, dummyterm);

	suite_runner = srunner_create(list_suite());
	srunner_add_suite(suite_runner, dict_suite());
	srunner_add_suite(suite_runner, listmodel_suite());
	srunner_add_suite(suite_runner, listview_suite());
	srunner_add_suite(suite_runner, path_suite());

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);

	delscreen(dummyscreen);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
