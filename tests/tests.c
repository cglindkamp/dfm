#include <check.h>
#include <stdlib.h>

Suite *list_suite(void);

int main(void)
{
	Suite *suite;
	SRunner *suite_runner;
	int number_failed;

	suite = list_suite();
	suite_runner = srunner_create(suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
