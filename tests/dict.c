/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <stdlib.h>

#include "../src/dict.h"
#include "tests.h"

char *__real_strdup(const char *s);

list_t *dict;

static void setup(void)
{
	dict = dict_new();
}

static void static_teardown(void)
{
	dict_delete(dict, false);
}

static void dynamic_teardown(void)
{
	dict_delete(dict, true);
}

START_TEST(test_dict_new)
{
	assert_oom(dict != NULL);

	ck_assert_int_eq(list_length(dict), 0);
}
END_TEST

START_TEST(test_dict_set_and_get_static_items)
{
	assert_oom(dict != NULL);

	assert_oom(dict_set(dict, "foo", "bar") == true);
	assert_oom(dict_set(dict, "bar", "baz") == true);
	assert_oom(dict_set(dict, "baz", "foo") == true);

	ck_assert_int_eq(list_length(dict), 3);
	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "bar"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "bar"), "baz"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "baz"), "foo"), 0);

	ck_assert(dict_set(dict, "foo", "baz") == true);
	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "baz"), 0);

	ck_assert_ptr_eq(dict_get(dict, "foobar"), NULL);
}
END_TEST


START_TEST(test_dict_set_and_get_dynamic_items)
{
	assert_oom(dict != NULL);

	char *foo = __real_strdup("foo");
	char *bar = __real_strdup("bar");
	char *baz = __real_strdup("baz");

	assert_oom_cleanup(dict_set(dict, "foo", bar) == true, free(bar), free(baz), free(foo));
	assert_oom_cleanup(dict_set(dict, "bar", baz) == true, free(baz), free(foo));
	assert_oom_cleanup(dict_set(dict, "baz", foo) == true, free(foo));

	ck_assert_int_eq(list_length(dict), 3);

	ck_assert_int_eq(strcmp(dict_get(dict, "foo"), "bar"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "bar"), "baz"), 0);
	ck_assert_int_eq(strcmp(dict_get(dict, "baz"), "foo"), 0);

	free(dict_get(dict, "foo"));
	ck_assert(dict_set(dict, "foo", __real_strdup("baz")) == true);
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
