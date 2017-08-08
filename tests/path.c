/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <errno.h>
#include <stdlib.h>

#include "wrapper/getcwd.h"
#include "../src/path.h"
#include "tests.h"

struct path path;

START_TEST(test_path_initialize)
{
	assert_oom(path_init(&path, 0) == true);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_addcomponent)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_add_component(&path, "foo") == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_addtwocomponents)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_add_component(&path, "foo") == true);
	assert_oom(path_add_component(&path, "bar") == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_removecomponent)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_add_component(&path, "foo") == true);
	assert_oom(path_add_component(&path, "bar") == true);
	ck_assert(path_remove_component(&path, NULL) == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_removelastcomponent)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_add_component(&path, "foo") == true);
	ck_assert(path_remove_component(&path, NULL) == true);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_removecomponentonroot)
{
	assert_oom(path_init(&path, 0) == true);
	ck_assert(path_remove_component(&path, NULL) == false);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_removecomponent_returnremovedcomponent)
{
	const char *component;
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_add_component(&path, "foo") == true);
	assert_oom(path_add_component(&path, "bar") == true);
	ck_assert(path_remove_component(&path, &component) == true);
	ck_assert_str_eq(component, "bar");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_removelastcomponent_returnremovedcomponent)
{
	const char *component;
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_add_component(&path, "foo") == true);
	ck_assert(path_remove_component(&path, &component) == true);
	ck_assert_str_eq(component, "foo");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_settocurrentworkingdirectory)
{
	assert_oom(path_init(&path, 0) == true);
	getcwd_setbehaviour("/foo/bar", false);
	assert_oom(path_set_to_current_working_directory(&path) == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_settocurrentworkingdirectory_getcwdfailed)
{
	assert_oom(path_init(&path, 0) == true);
	getcwd_setbehaviour(NULL, true);
	ck_assert(path_set_to_current_working_directory(&path) == false);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_setfromstring)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_set_from_string(&path, "/foo/bar") == 0);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_setfromstring_reduceslashes)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_set_from_string(&path, "//foo////bar/") == 0);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_setfromstring_illegalpath)
{
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_set_from_string(&path, "foo/bar") == EINVAL);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_destroy(&path);
}
END_TEST

const char *home;

static void restore_environment(const char *name, const char *value)
{
	if(value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}

static void setup(void)
{
	home = getenv("HOME");
}

static void teardown(void)
{
	restore_environment("HOME", home);
}

START_TEST(test_path_setfromstring_homedirectory)
{
	setenv("HOME", "/home/dfm", 1);
	assert_oom(path_init(&path, 0) == true);
	assert_oom(path_set_from_string(&path, "~") == 0);
	ck_assert_str_eq(path_tocstr(&path), "/home/dfm");
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_setfromstring_homedirectory_unset)
{
	unsetenv("HOME");
	assert_oom(path_init(&path, 0) == true);
	ck_assert_int_eq(path_set_from_string(&path, "~"), EINVAL);
	path_destroy(&path);
}
END_TEST

START_TEST(test_path_setfromstring_homedirectory_tilde)
{
	setenv("HOME", "~", 1);
	assert_oom(path_init(&path, 0) == true);
	ck_assert_int_eq(path_set_from_string(&path, "~"), EINVAL);
	path_destroy(&path);
}
END_TEST

Suite *path_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Path");

	tcase = tcase_create("Core");
	tcase_add_test(tcase, test_path_initialize);
	tcase_add_test(tcase, test_path_addcomponent);
	tcase_add_test(tcase, test_path_addtwocomponents);
	tcase_add_test(tcase, test_path_removecomponent);
	tcase_add_test(tcase, test_path_removelastcomponent);
	tcase_add_test(tcase, test_path_removecomponentonroot);
	tcase_add_test(tcase, test_path_removecomponent_returnremovedcomponent);
	tcase_add_test(tcase, test_path_removelastcomponent_returnremovedcomponent);
	tcase_add_test(tcase, test_path_settocurrentworkingdirectory);
	tcase_add_test(tcase, test_path_settocurrentworkingdirectory_getcwdfailed);
	tcase_add_test(tcase, test_path_setfromstring);
	tcase_add_test(tcase, test_path_setfromstring_reduceslashes);
	tcase_add_test(tcase, test_path_setfromstring_illegalpath);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("Home Directory");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_path_setfromstring_homedirectory);
	tcase_add_test(tcase, test_path_setfromstring_homedirectory_unset);
	tcase_add_test(tcase, test_path_setfromstring_homedirectory_tilde);
	suite_add_tcase(suite, tcase);

	return suite;
}
