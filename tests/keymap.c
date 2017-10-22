/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../src/application.h"
#include "../src/keymap.h"
#include "tests.h"

static struct application *app;
static const char *param;
static int command;

static void command1(struct application *a, const char *p)
{
	app = a;
	command = 1;
	param = p;
}

static void command2(struct application *a, const char *p)
{
	app = a;
	command = 2;
	param = p;
}

static void command4(struct application *a, const char *p)
{
	app = a;
	command = 4;
	param = p;
}

static struct command_map command_map[] = {
	{ "command1", command1, false },
	{ "command2", command2, false },
	{ "command4", command4, true },
	{ NULL, NULL, false },
};

static struct {
	const char *string;
	struct keymap keymap;
} singlelinetesttable[] = {
	{ "a command1", { .keyspec = { .key = L'a', .iskeycode = false }, .command = command1, .param = NULL } },
	{ "a \t  command1", { .keyspec = { .key = L'a', .iskeycode = false }, .command = command1, .param = NULL } },
	{ "b command2 foo", { .keyspec = { .key = L'b', .iskeycode = false }, .command = command2, .param = "foo" } },
	{ "b command4 foo", { .keyspec = { .key = L'b', .iskeycode = false }, .command = command4, .param = "foo" } },
	{ "  b command2  foo bar", { .keyspec = { .key = L'b', .iskeycode = false }, .command = command2, .param = "foo bar" } },
	{ u8"ü command1", { .keyspec = { .key = L'ü', .iskeycode = false }, .command = command1, .param = NULL } },
	{ "space command2", { .keyspec = { .key = L' ', .iskeycode = false }, .command = command2, .param = NULL } },
	{ "up command2", { .keyspec = { .key = KEY_UP, .iskeycode = true }, .command = command2, .param = NULL } },
};

START_TEST(test_keymap_singleline)
{
	struct keymap *keymap;
	char keymapstring[strlen(singlelinetesttable[_i].string) + 1];
	strcpy(keymapstring, singlelinetesttable[_i].string);

	int ret = keymap_newfromstring(&keymap, keymapstring, command_map);

	assert_oom(ret != ENOMEM);
	ck_assert(keymap != NULL);
	ck_assert(keymap[0].keyspec.iskeycode == singlelinetesttable[_i].keymap.keyspec.iskeycode);
	ck_assert(keymap[0].keyspec.key == singlelinetesttable[_i].keymap.keyspec.key);
	ck_assert(keymap[0].command == singlelinetesttable[_i].keymap.command);
	if(singlelinetesttable[_i].keymap.param == NULL)
		ck_assert_ptr_eq(keymap[0].param, singlelinetesttable[_i].keymap.param);
	else
		ck_assert_str_eq(keymap[0].param, singlelinetesttable[_i].keymap.param);

	keymap_delete(keymap);
}
END_TEST

const char *singlelinetesttable_invalid[] = {
	"",
	"a",
	"unknownkey command2",
	"a command3",
	"a command4",
};

START_TEST(test_keymap_singleline_invalid)
{
	struct keymap *keymap;
	char keymapstring[strlen(singlelinetesttable_invalid[_i]) + 1];
	strcpy(keymapstring, singlelinetesttable_invalid[_i]);

	int ret = keymap_newfromstring(&keymap, keymapstring, command_map);

	assert_oom(ret != ENOMEM);
	ck_assert_int_eq(ret, EINVAL);
}
END_TEST

START_TEST(test_keymap_multiline)
{
	struct keymap *keymap;
	char keymapstring[] = "a command1\n \nup command2 bar";

	int ret = keymap_newfromstring(&keymap, keymapstring, command_map);

	assert_oom(ret != ENOMEM);
	ck_assert(keymap != NULL);

	ck_assert(keymap[0].keyspec.iskeycode == false);
	ck_assert(keymap[0].keyspec.key == L'a');
	ck_assert(keymap[0].command == command1);
	ck_assert_ptr_eq(keymap[0].param, NULL);

	ck_assert(keymap[1].keyspec.iskeycode == true);
	ck_assert(keymap[1].keyspec.key == KEY_UP);
	ck_assert(keymap[1].command == command2);
	ck_assert_str_eq(keymap[1].param, "bar");

	keymap_delete(keymap);
}
END_TEST

START_TEST(test_keymap_handlekeys)
{
	struct keymap *keymap;
	char keymapstring[] = "a command1\nup command2 bar";

	int ret = keymap_newfromstring(&keymap, keymapstring, command_map);

	assert_oom(ret != ENOMEM);
	ck_assert(keymap != NULL);

	app = NULL;
	param = "foo";
	command = 0;
	keymap_handlekey(keymap, (struct application *)0xfe, L'a', false);
	ck_assert_ptr_eq(app, (struct application *)0xfe);
	ck_assert_int_eq(command, 1);
	ck_assert_ptr_eq(param, NULL);

	app = NULL;
	param = "foo";
	command = 0;
	keymap_handlekey(keymap, (struct application *)0xff, KEY_UP, true);
	ck_assert_ptr_eq(app, (struct application *)0xff);
	ck_assert_int_eq(command, 2);
	ck_assert_str_eq(param, "bar");

	keymap_delete(keymap);
}
END_TEST

START_TEST(test_keymap_deletenull)
{
	keymap_delete(NULL);
}
END_TEST

START_TEST(test_keymap_nonexistantfile)
{
	struct keymap *keymap;

	int ret = keymap_newfromfile(&keymap, "examples/nonexistantfile", command_map);

	assert_oom(ret != ENOMEM);
	ck_assert_int_eq(ret, ENOENT);
}
END_TEST

START_TEST(test_keymap_inaccessablefile)
{
	struct keymap *keymap;

	char path[] = "/tmp/keymap.XXXXXX";
	mkstemp(path);
	chmod(path, 0);
	int ret = keymap_newfromfile(&keymap, path, command_map);

	assert_oom_cleanup(ret != ENOMEM, remove(path));
	ck_assert_int_eq(ret, EACCES);

	remove(path);
}
END_TEST

extern struct command_map application_command_map[];

START_TEST(test_keymap_examplefile)
{
	struct keymap *keymap;

	int ret = keymap_newfromfile(&keymap, "examples/keymap", application_command_map);

	assert_oom(ret != ENOMEM);
	ck_assert_int_eq(ret, 0);
	ck_assert_ptr_ne(keymap, NULL);

	keymap_delete(keymap);
}
END_TEST

Suite *keymap_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Keymap");

	tcase = tcase_create("Core");
	tcase_add_loop_test(tcase, test_keymap_singleline, 0, sizeof(singlelinetesttable)/sizeof(singlelinetesttable[0]));
	tcase_add_loop_test(tcase, test_keymap_singleline_invalid, 0, sizeof(singlelinetesttable_invalid)/sizeof(singlelinetesttable_invalid[0]));
	tcase_add_test(tcase, test_keymap_multiline);
	tcase_add_test(tcase, test_keymap_deletenull);
	tcase_add_test(tcase, test_keymap_handlekeys);
	tcase_add_test(tcase, test_keymap_nonexistantfile);
	tcase_add_test(tcase, test_keymap_inaccessablefile);
	tcase_add_test(tcase, test_keymap_examplefile);
	suite_add_tcase(suite, tcase);

	return suite;
}
