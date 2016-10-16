#include <check.h>
#include <stdlib.h>

#include "../src/dict.h"

list_t *dict;

static void setup(void)
{
	dict = dict_new();
}

static void static_teardown(void)
{
	dict_free(dict, false);
	dict = NULL;
}

static void dynamic_teardown(void)
{
	dict_free(dict, true);
	dict = NULL;
}

START_TEST(test_dict_new)
{
	ck_assert_int_eq(list_length(dict), 0);
}
END_TEST

START_TEST(test_dict_set_and_get_static_items)
{
	dict_set(dict, "foo", "bar");
	dict_set(dict, "bar", "baz");
	dict_set(dict, "baz", "foo");

	ck_assert_int_eq(list_length(dict), 3);

	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "bar"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "bar"), "baz"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "baz"), "foo"), 0);

	dict_set(dict, "foo", "baz");
	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "baz"), 0);

	ck_assert_ptr_eq(dict_get(dict, "foobar"), NULL);
}
END_TEST


START_TEST(test_dict_set_and_get_dynamic_items)
{
	dict_set(dict, "foo", strdup("bar"));
	dict_set(dict, "bar", strdup("baz"));
	dict_set(dict, "baz", strdup("foo"));

	ck_assert_int_eq(list_length(dict), 3);

	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "bar"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "bar"), "baz"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "baz"), "foo"), 0);

	free(dict_get(dict, "foo"));
	dict_set(dict, "foo", strdup("baz"));
	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "baz"), 0);

	ck_assert_ptr_eq(dict_get(dict, "foobar"), NULL);
}
END_TEST

Suite *dict_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Dict");

	tcase = tcase_create("Static Items");
	tcase_add_checked_fixture(tcase, setup, static_teardown);
	tcase_add_test(tcase, test_dict_new);
	tcase_add_test(tcase, test_dict_set_and_get_static_items);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("Dynamic Items");
	tcase_add_checked_fixture(tcase, setup, dynamic_teardown);
	tcase_add_test(tcase, test_dict_new);
	tcase_add_test(tcase, test_dict_set_and_get_dynamic_items);
	suite_add_tcase(suite, tcase);
	return suite;
}
