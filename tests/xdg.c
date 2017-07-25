/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/list.h"
#include "../src/path.h"
#include "../src/xdg.h"
#include "tests.h"

const char *home;
const char *config_home;
const char *config_dirs;

void restore_environment(const char *name, const char *value)
{
	if(value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}

void setup(void)
{
	home = getenv("HOME");
	config_home = getenv("XDG_CONFIG_HOME");
	config_dirs = getenv("XDG_CONFIG_DIRS");
}

void teardown(void)
{
	restore_environment("HOME", home);
	restore_environment("XDG_CONFIG_HOME", config_home);
	restore_environment("XDG_CONFIG_DIRS", config_dirs);
}

START_TEST(test_xdg_confighome_confighomeset)
{
	struct path *path;

	setenv("XDG_CONFIG_HOME", "/foo/bar", 1);

	int ret = xdg_get_config_home(&path);
	assert_oom(ret == 0);
	ck_assert_str_eq(path_tocstr(path), "/foo/bar");
	path_delete(path);
}
END_TEST

START_TEST(test_xdg_confighome_confighomeunset)
{
	struct path *path;

	setenv("HOME", "/foo/bar", 1);
	unsetenv("XDG_CONFIG_HOME");

	int ret = xdg_get_config_home(&path);
	assert_oom(ret == 0);
	ck_assert_str_eq(path_tocstr(path), "/foo/bar/.config");
	path_delete(path);
}
END_TEST

START_TEST(test_xdg_confighome_confighomeinvalid)
{
	struct path *path;

	setenv("HOME", "/foo/bar", 1);
	setenv("XDG_CONFIG_HOME", "bar/baz", 1);

	int ret = xdg_get_config_home(&path);
	assert_oom(ret == 0);
	ck_assert_str_eq(path_tocstr(path), "/foo/bar/.config");
	path_delete(path);
}
END_TEST

START_TEST(test_xdg_confighome_confighomeandhomeunset)
{
	struct path *path;

	unsetenv("HOME");
	unsetenv("XDG_CONFIG_HOME");

	int ret = xdg_get_config_home(&path);
	ck_assert_int_eq(ret, ENOENT);
	ck_assert(path == NULL);
}
END_TEST

START_TEST(test_xdg_confighome_confighomeandhomeinvalid)
{
	struct path *path;

	setenv("HOME", "foo/bar", 1);
	setenv("XDG_CONFIG_HOME", "bar/baz", 1);

	int ret = xdg_get_config_home(&path);
	ck_assert_int_eq(ret, ENOENT);
	ck_assert(path == NULL);
}
END_TEST

START_TEST(test_xdg_configdirs_unset)
{
	unsetenv("XDG_CONFIG_DIRS");

	list_t *list = xdg_get_config_dirs(false);
	assert_oom(list != NULL);

	ck_assert_uint_eq(list_length(list), 1);

	struct path *path = list_get_item(list, 0);
	ck_assert_str_eq(path_tocstr(path), "/etc/xdg");

	list_delete(list, (list_item_deallocator)path_delete);
}
END_TEST

START_TEST(test_xdg_configdirs_set)
{
	setenv("XDG_CONFIG_DIRS", "/foo:/bar:/baz", 1);

	list_t *list = xdg_get_config_dirs(false);
	assert_oom(list != NULL);

	ck_assert_uint_eq(list_length(list), 3);

	struct path *path = list_get_item(list, 0);
	ck_assert_str_eq(path_tocstr(path), "/foo");

	path = list_get_item(list, 1);
	ck_assert_str_eq(path_tocstr(path), "/bar");

	path = list_get_item(list, 2);
	ck_assert_str_eq(path_tocstr(path), "/baz");

	list_delete(list, (list_item_deallocator)path_delete);
}
END_TEST

START_TEST(test_xdg_configdirs_partlyinvalid)
{
	setenv("XDG_CONFIG_DIRS", "/foo:bar:/baz", 1);

	list_t *list = xdg_get_config_dirs(false);
	assert_oom(list != NULL);

	ck_assert_uint_eq(list_length(list), 2);

	struct path *path = list_get_item(list, 0);
	ck_assert_str_eq(path_tocstr(path), "/foo");

	path = list_get_item(list, 1);
	ck_assert_str_eq(path_tocstr(path), "/baz");

	list_delete(list, (list_item_deallocator)path_delete);
}
END_TEST

START_TEST(test_xdg_configdirs_completelyinvalid)
{
	setenv("XDG_CONFIG_DIRS", "foo:bar:baz", 1);

	list_t *list = xdg_get_config_dirs(false);
	assert_oom(list != NULL);

	ck_assert_uint_eq(list_length(list), 1);

	struct path *path = list_get_item(list, 0);
	ck_assert_str_eq(path_tocstr(path), "/etc/xdg");

	list_delete(list, (list_item_deallocator)path_delete);
}
END_TEST

START_TEST(test_xdg_configdirs_includeconfighome)
{
	setenv("XDG_CONFIG_HOME", "/foo", 1);
	setenv("XDG_CONFIG_DIRS", "/bar:/baz", 1);

	list_t *list = xdg_get_config_dirs(true);
	assert_oom(list != NULL);

	ck_assert_uint_eq(list_length(list), 3);

	struct path *path = list_get_item(list, 0);
	ck_assert_str_eq(path_tocstr(path), "/foo");

	path = list_get_item(list, 1);
	ck_assert_str_eq(path_tocstr(path), "/bar");

	path = list_get_item(list, 2);
	ck_assert_str_eq(path_tocstr(path), "/baz");

	list_delete(list, (list_item_deallocator)path_delete);
}
END_TEST

START_TEST(test_xdg_configdirs_includeconfighome_homeinvalid)
{
	setenv("HOME", "foo", 1);
	setenv("XDG_CONFIG_HOME", "foo", 1);
	setenv("XDG_CONFIG_DIRS", "/bar:/baz", 1);

	list_t *list = xdg_get_config_dirs(true);
	assert_oom(list != NULL);

	ck_assert_uint_eq(list_length(list), 2);

	struct path *path = list_get_item(list, 0);
	ck_assert_str_eq(path_tocstr(path), "/bar");

	path = list_get_item(list, 1);
	ck_assert_str_eq(path_tocstr(path), "/baz");

	list_delete(list, (list_item_deallocator)path_delete);
}
END_TEST

Suite *xdg_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("XDG");

	tcase = tcase_create("Core");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_xdg_confighome_confighomeset);
	tcase_add_test(tcase, test_xdg_confighome_confighomeunset);
	tcase_add_test(tcase, test_xdg_confighome_confighomeinvalid);
	tcase_add_test(tcase, test_xdg_confighome_confighomeandhomeunset);
	tcase_add_test(tcase, test_xdg_confighome_confighomeandhomeinvalid);
	tcase_add_test(tcase, test_xdg_configdirs_unset);
	tcase_add_test(tcase, test_xdg_configdirs_set);
	tcase_add_test(tcase, test_xdg_configdirs_partlyinvalid);
	tcase_add_test(tcase, test_xdg_configdirs_completelyinvalid);
	tcase_add_test(tcase, test_xdg_configdirs_includeconfighome);
	tcase_add_test(tcase, test_xdg_configdirs_includeconfighome_homeinvalid);
	suite_add_tcase(suite, tcase);

	return suite;
}
