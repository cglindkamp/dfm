/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <errno.h>
#include <ncurses.h>

#include "../src/commandline.h"
#include "../src/keymap.h"
#include "tests.h"

static struct {
	struct keyspec *keypresses;
	size_t keypresscount;
	const wchar_t *result;
	const wchar_t *output;
} testdata[] = {
	{
		.keypresses = NULL,
		.keypresscount = 0,
		.result = L"",
		.output = L":         ",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'} },
		.keypresscount = 5,
		.result = L"hello",
		.output = L":hello    ",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 1, KEY_BACKSPACE}, { 1, KEY_BACKSPACE} },
		.keypresscount = 4,
		.result = L"",
		.output = L":         ",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'} },
		.keypresscount = 11,
		.result = L"hello world",
		.output = L":lo world ",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_BACKSPACE} },
		.keypresscount = 12,
		.result = L"hello worl",
		.output = L":llo worl ",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_LEFT}, {1, KEY_BACKSPACE} },
		.keypresscount = 13,
		.result = L"hello word",
		.output = L":ello word",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_LEFT}, {0, L'a'} },
		.keypresscount = 13,
		.result = L"hello worlad",
		.output = L":lo worlad",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_LEFT}, {1, KEY_LEFT}, {1, KEY_RIGHT}, {0, L'a'} },
		.keypresscount = 15,
		.result = L"hello worlad",
		.output = L":lo worlad",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_HOME}, {0, L'a'} },
		.keypresscount = 13,
		.result = L"ahello world",
		.output = L":ahello wo",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_HOME}, { 1, KEY_END}, {0, L'a'} },
		.keypresscount = 14,
		.result = L"hello worlda",
		.output = L":o worlda ",
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_HOME}, {1, KEY_DC} },
		.keypresscount = 13,
		.result = L"ello world",
		.output = L":ello worl",
	},
};

START_TEST(test_commandline_input)
{
	struct commandline cmdline;

	commandline_init(&cmdline, 0, 0, 10);
	assert_oom(commandline_start(&cmdline, L':') != ENOMEM);

	for(size_t i = 0; i < testdata[_i].keypresscount; i++)
		assert_oom_cleanup(commandline_handlekey(&cmdline, testdata[_i].keypresses[i].key, testdata[_i].keypresses[i].iskeycode) != ENOMEM, commandline_destroy(&cmdline));

	const wchar_t *result = commandline_getcommand(&cmdline);
	ck_assert(wcscmp(result, testdata[_i].result) == 0);

	wchar_t buffer[10];
	mvwinnwstr(cmdline.window, 0, 0, buffer, 10);
	ck_assert(wcscmp(buffer, testdata[_i].output) == 0);

	commandline_destroy(&cmdline);
}
END_TEST

Suite *commandline_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("Command Line");

	tcase = tcase_create("Core");
	tcase_add_loop_test(tcase, test_commandline_input, 0, sizeof(testdata)/sizeof(testdata[0]));
	suite_add_tcase(suite, tcase);

	return suite;
}
