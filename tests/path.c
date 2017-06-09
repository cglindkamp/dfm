/* See LICENSE file for copyright and license details. */
#include <check.h>

#include "wrapper/getcwd.h"
#include "../src/path.h"

struct path path;

START_TEST(test_path_initialize)
{
	path_init(&path, 0);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_free(&path);
}
END_TEST

START_TEST(test_path_addcomponent)
{
	path_init(&path, 0);
	path_add_component(&path, "foo");
	ck_assert_str_eq(path_tocstr(&path), "/foo");
	path_free(&path);
}
END_TEST

START_TEST(test_path_addtwocomponents)
{
	path_init(&path, 0);
	path_add_component(&path, "foo");
	path_add_component(&path, "bar");
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_free(&path);
}
END_TEST

START_TEST(test_path_removecomponent)
{
	path_init(&path, 0);
	path_add_component(&path, "foo");
	path_add_component(&path, "bar");
	ck_assert(path_remove_component(&path, NULL) == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo");
	path_free(&path);
}
END_TEST

START_TEST(test_path_removelastcomponent)
{
	path_init(&path, 0);
	path_add_component(&path, "foo");
	ck_assert(path_remove_component(&path, NULL) == true);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_free(&path);
}
END_TEST

START_TEST(test_path_removecomponentonroot)
{
	path_init(&path, 0);
	ck_assert(path_remove_component(&path, NULL) == false);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_free(&path);
}
END_TEST

START_TEST(test_path_removecomponent_returnremovedcomponent)
{
	const char *component;
	path_init(&path, 0);
	path_add_component(&path, "foo");
	path_add_component(&path, "bar");
	ck_assert(path_remove_component(&path, &component) == true);
	ck_assert_str_eq(component, "bar");
	path_free(&path);
}
END_TEST

START_TEST(test_path_removelastcomponent_returnremovedcomponent)
{
	const char *component;
	path_init(&path, 0);
	path_add_component(&path, "foo");
	ck_assert(path_remove_component(&path, &component) == true);
	ck_assert_str_eq(component, "foo");
	path_free(&path);
}
END_TEST

START_TEST(test_path_settocurrentworkingdirectory)
{
	path_init(&path, 0);
	getcwd_setbehaviour("/foo/bar", false);
	ck_assert(path_set_to_current_working_directory(&path) == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_free(&path);
}
END_TEST

START_TEST(test_path_settocurrentworkingdirectory_getcwdfailed)
{
	path_init(&path, 0);
	getcwd_setbehaviour(NULL, true);
	ck_assert(path_set_to_current_working_directory(&path) == false);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_free(&path);
}
END_TEST

START_TEST(test_path_setfromstring)
{
	path_init(&path, 0);
	ck_assert(path_set_from_string(&path, "/foo/bar") == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_free(&path);
}
END_TEST

START_TEST(test_path_setfromstring_reduceslashes)
{
	path_init(&path, 0);
	ck_assert(path_set_from_string(&path, "//foo////bar/") == true);
	ck_assert_str_eq(path_tocstr(&path), "/foo/bar");
	path_free(&path);
}
END_TEST

START_TEST(test_path_setfromstring_illegalpath)
{
	path_init(&path, 0);
	ck_assert(path_set_from_string(&path, "foo/bar") == false);
	ck_assert_str_eq(path_tocstr(&path), "/");
	path_free(&path);
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

	return suite;
}
