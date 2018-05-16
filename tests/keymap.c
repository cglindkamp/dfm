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

static void command1(struct commandexecutor *c, char *p)
{
	cmdex = c;
	command = 1;
	if(p)
		strcpy(param, p);
}

static void command2(struct commandexecutor *c, char *p)
{
	cmdex = c;
	command = 2;
	if(p)
		strcpy(param, p);
}

static void command4(struct commandexecutor *c, char *p)
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
	int ret = keymap_addmapping(&keymap, keymapstring);

	assert_oom_cleanup(ret != ENOMEM, keymap_destroy(&keymap));
	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	ck_assert_int_eq(keymap_handlekey(&keymap, singlelinetesttable[_i].keyspec.key, singlelinetesttable[_i].keyspec.iskeycode), 0);
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
	int ret = keymap_addmapping(&keymap, keymapstring);

	assert_oom_cleanup(ret != ENOMEM, keymap_destroy(&keymap));
	ck_assert_int_eq(ret, EINVAL);
	keymap_destroy(&keymap);
}
END_TEST

START_TEST(test_keymap_multiline)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;
	char keymapstring1[] = "a command1";
	char keymapstring2[] = "up command2 bar";

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);

	int ret = keymap_addmapping(&keymap, keymapstring1);
	assert_oom_cleanup(ret != ENOMEM, keymap_destroy(&keymap));

	ret = keymap_addmapping(&keymap, keymapstring2);
	assert_oom_cleanup(ret != ENOMEM, keymap_destroy(&keymap));

	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	ck_assert_int_eq(keymap_handlekey(&keymap, L'a', false), 0);
	ck_assert_ptr_eq(cmdex, &commandexecutor);
	ck_assert_int_eq(command, 1);
	ck_assert_str_eq(param, "");

	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	ck_assert_int_eq(keymap_handlekey(&keymap, KEY_UP, true), 0);
	ck_assert_ptr_eq(cmdex, &commandexecutor);
	ck_assert_int_eq(command, 2);
	ck_assert_str_eq(param, "bar");

	keymap_destroy(&keymap);
}
END_TEST

START_TEST(test_keymap_duplicate_entry)
{
	struct commandexecutor commandexecutor;
	struct keymap keymap;
	char keymapstring1[] = "a command1";
	char keymapstring2[] = "a command2 bar";

	commandexecutor_init(&commandexecutor, command_map);
	keymap_init(&keymap, &commandexecutor);

	int ret = keymap_addmapping(&keymap, keymapstring1);
	assert_oom_cleanup(ret != ENOMEM, keymap_destroy(&keymap));

	ret = keymap_addmapping(&keymap, keymapstring2);
	assert_oom_cleanup(ret != ENOMEM, keymap_destroy(&keymap));

	cmdex = NULL;
	param[0] = '\0';
	command = 0;
	ck_assert_int_eq(keymap_handlekey(&keymap, L'a', false), 0);
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

Suite *keymap_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Keymap");

	tcase = tcase_create("Core");
	tcase_add_loop_test(tcase, test_keymap_singleline, 0, sizeof(singlelinetesttable)/sizeof(singlelinetesttable[0]));
	tcase_add_loop_test(tcase, test_keymap_singleline_invalid, 0, sizeof(singlelinetesttable_invalid)/sizeof(singlelinetesttable_invalid[0]));
	tcase_add_test(tcase, test_keymap_multiline);
	tcase_add_test(tcase, test_keymap_duplicate_entry);
	tcase_add_test(tcase, test_keymap_deletenull);
	suite_add_tcase(suite, tcase);

	return suite;
}
