/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../src/application.h"
#include "../src/commandexecutor.h"
#include "../src/keymap.h"
#include "tests.h"

static struct commandexecutor *cmdex;
static char param[10];
static int command;

static void command1(struct commandexecutor *c, const char *p)
{
	cmdex = c;
	command = 1;
	if(p)
		strcpy(param, p);
}

static void command2(struct commandexecutor *c, const char *p)
{
	cmdex = c;
	command = 2;
	if(p)
		strcpy(param, p);
}

static void command4(struct commandexecutor *c, const char *p)
{
	cmdex = c;
	command = 4;
	if(p)
		strcpy(param, p);
}

static struct command_map command_map[] = {
	{ "command1", command1, false },
	{ "command2", command2, false },
	{ "command4", command4, true },
	{ NULL, NULL, false },
};

static struct {
	const char *string;
	struct keyspec keyspec;
	int cmdnr;
	const char *param;
} singlelinetesttable[] = {
	{ "a command1", .keyspec = { .key = L'a', .iskeycode = false }, .cmdnr =1, .param = NULL },
	{ "a \t  command1", .keyspec = { .key = L'a', .iskeycode = false }, .cmdnr = 1, .param = NULL },
	{ "b command2 foo", .keyspec = { .key = L'b', .iskeycode = false }, .cmdnr = 2, .param = "foo" },
	{ "b command4 foo", .keyspec = { .key = L'b', .iskeycode = false }, .cmdnr = 4, .param = "foo" },
	{ "  b command2  foo bar", .keyspec = { .key = L'b', .iskeycode = false }, .cmdnr = 2, .param ="foo bar" },
	{ u8"ü command1", .keyspec = { .key = L'ü', .iskeycode = false }, .cmdnr = 1, .param = NULL },
	{ "space command2", .keyspec = { .key = L' ', .iskeycode = false }, .cmdnr = 2, .param = NULL },
	{ "up command2", .keyspec = { .key = KEY_UP, .iskeycode = true }, .cmdnr = 2, .param = NULL },
};

START_TEST(test_keymap_singleline)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;
	char keymapstring[strlen(singlelinetesttable[_i].string) + 1];
	strcpy(keymapstring, singlelinetesttable[_i].string);

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);
	int ret = keymap_setfromstring(&keymap, keymapstring);

	assert_oom(ret != ENOMEM);
	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	assert_oom(keymap_handlekey(&keymap, singlelinetesttable[_i].keyspec.key, singlelinetesttable[_i].keyspec.iskeycode) != ENOMEM);
	ck_assert_ptr_eq(cmdex, &commandexecutor);
	ck_assert_int_eq(command, singlelinetesttable[_i].cmdnr);
	if(singlelinetesttable[_i].param == NULL)
		ck_assert_str_eq(param, "");
	else
		ck_assert_str_eq(param, singlelinetesttable[_i].param);

	keymap_destroy(&keymap);
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
	struct commandexecutor commandexecutor;
	struct keymap keymap;
	char keymapstring[strlen(singlelinetesttable_invalid[_i]) + 1];
	strcpy(keymapstring, singlelinetesttable_invalid[_i]);

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);
	int ret = keymap_setfromstring(&keymap, keymapstring);

	assert_oom(ret != ENOMEM);
	ck_assert_int_eq(ret, EINVAL);
}
END_TEST

START_TEST(test_keymap_multiline)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;
	char keymapstring[] = "a command1\nup command2 bar";

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);
	int ret = keymap_setfromstring(&keymap, keymapstring);

	assert_oom(ret != ENOMEM);

	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	assert_oom(keymap_handlekey(&keymap, L'a', false) != ENOMEM);
	ck_assert_ptr_eq(cmdex, &commandexecutor);
	ck_assert_int_eq(command, 1);
	ck_assert_str_eq(param, "");

	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	assert_oom(keymap_handlekey(&keymap, KEY_UP, true) != ENOMEM);
	ck_assert_ptr_eq(cmdex, &commandexecutor);
	ck_assert_int_eq(command, 2);
	ck_assert_str_eq(param, "bar");

	keymap_destroy(&keymap);
}
END_TEST

START_TEST(test_keymap_deletenull)
{
	keymap_destroy(NULL);
}
END_TEST

START_TEST(test_keymap_nonexistantfile)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);
	int ret = keymap_setfromfile(&keymap, "examples/nonexistantfile");

	assert_oom(ret != ENOMEM);
	ck_assert_int_eq(ret, ENOENT);

	keymap_destroy(&keymap);
}
END_TEST

START_TEST(test_keymap_inaccessablefile)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;

	char path[] = "/tmp/keymap.XXXXXX";
	ck_assert_int_ne(mkstemp(path), -1);
	chmod(path, 0);

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);
	int ret = keymap_setfromfile(&keymap, path);

	assert_oom_cleanup(ret != ENOMEM, remove(path));
	ck_assert_int_eq(ret, EACCES);

	remove(path);

	keymap_destroy(&keymap);
}
END_TEST

extern struct command_map application_command_map[];

START_TEST(test_keymap_examplefile)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;

	commandexecutor_init(&commandexecutor, application_command_map);
	keymap_init(&keymap, &commandexecutor);
	int ret = keymap_setfromfile(&keymap, "examples/keymap");

	assert_oom(ret != ENOMEM);
	ck_assert_int_eq(ret, 0);

	keymap_destroy(&keymap);
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
	tcase_add_test(tcase, test_keymap_nonexistantfile);
	tcase_add_test(tcase, test_keymap_inaccessablefile);
	tcase_add_test(tcase, test_keymap_examplefile);
	suite_add_tcase(suite, tcase);

	return suite;
}
