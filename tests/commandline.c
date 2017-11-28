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
	size_t cursorpos;
} testdata[] = {
	{
		.keypresses = NULL,
		.keypresscount = 0,
		.result = L"",
		.output = L":         ",
		.cursorpos = 1,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'} },
		.keypresscount = 5,
		.result = L"hello",
		.output = L":hello    ",
		.cursorpos = 6,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 1, KEY_BACKSPACE}, { 1, KEY_BACKSPACE} },
		.keypresscount = 4,
		.result = L"",
		.output = L":         ",
		.cursorpos = 1,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'} },
		.keypresscount = 11,
		.result = L"hello world",
		.output = L":lo world ",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_BACKSPACE} },
		.keypresscount = 12,
		.result = L"hello worl",
		.output = L":llo worl ",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_LEFT}, {1, KEY_BACKSPACE} },
		.keypresscount = 13,
		.result = L"hello word",
		.output = L":ello word",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_LEFT}, {0, L'a'} },
		.keypresscount = 13,
		.result = L"hello worlad",
		.output = L":lo worlad",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_LEFT}, {1, KEY_LEFT}, {1, KEY_RIGHT}, {0, L'a'} },
		.keypresscount = 15,
		.result = L"hello worlad",
		.output = L":lo worlad",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_HOME}, {0, L'a'} },
		.keypresscount = 13,
		.result = L"ahello world",
		.output = L":ahello wo",
		.cursorpos = 2,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_HOME}, { 1, KEY_END}, {0, L'a'} },
		.keypresscount = 14,
		.result = L"hello worlda",
		.output = L":o worlda ",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'h' }, { 0, L'e'}, { 0, L'l'}, { 0, L'l'}, { 0, L'o'}, { 0, L' ' }, { 0, L'w'}, { 0, L'o'}, { 0, L'r'}, { 0, L'l'}, { 0, L'd'}, { 1, KEY_HOME}, {1, KEY_DC} },
		.keypresscount = 13,
		.result = L"ello world",
		.output = L":ello worl",
		.cursorpos = 1,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, 0x300 }, { 0, 0x301 } },
		.keypresscount = 3,
		.result = L"à́",
		.output = L":à́        ",
		.cursorpos = 2,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x4f60 }, { 0, 0x597d } },
		.keypresscount = 2,
		.result = L"你好",
		.output = L":你好     ",
		.cursorpos = 5,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, 0x300 }, { 0, 0x301 }, { 1, KEY_LEFT } },
		.keypresscount = 4,
		.result = L"à́",
		.output = L":à́        ",
		.cursorpos = 1,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, 0x300 }, { 0, 0x301 }, { 0, L'a' }, { 1, KEY_HOME }, { 1, KEY_RIGHT }, { 1, KEY_RIGHT } },
		.keypresscount = 7,
		.result = L"à́a",
		.output = L":à́a       ",
		.cursorpos = 3,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, L'a' }, { 0, 0x300 }, { 0, 0x301 }, { 1, KEY_BACKSPACE } },
		.keypresscount = 5,
		.result = L"a",
		.output = L":a        ",
		.cursorpos = 2,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, 0x300 }, { 0, 0x301 }, { 0, L'a' }, { 1, KEY_HOME }, { 1, KEY_DC } },
		.keypresscount = 6,
		.result = L"a",
		.output = L":a        ",
		.cursorpos = 1,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, 0x300 }, { 0, 0x301 }, { 0, 0x302 }, { 0, 0x303 }, { 0, L'a' }, { 0, 0x304 }, { 0, 0x305 }, { 0, 0x306 }, { 0, 0x307 } },
		.keypresscount = 10,
		.result = L"à́̂̃ā̅̆̇",
		.output = L":à́̂̃ā̅̆̇       ",
		.cursorpos = 3,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d } },
		.keypresscount = 6,
		.result = L"你好你好你好",
		.output = L":你好你好 ",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, L'a' } },
		.keypresscount = 7,
		.result = L"你好你好你好a",
		.output = L":好你好a  ",
		.cursorpos = 8,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, L'a' }, { 0, L'a' } },
		.keypresscount = 8,
		.result = L"你好你好你好aa",
		.output = L":好你好aa ",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, L'a' }, { 0, L'a' }, { 1, KEY_BACKSPACE } },
		.keypresscount = 9,
		.result = L"你好你好你好a",
		.output = L":好你好a  ",
		.cursorpos = 8,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, 0x4f60 }, { 0, 0x597d }, { 0, L'a' }, { 0, L'a' }, { 1, KEY_LEFT } },
		.keypresscount = 9,
		.result = L"你好你好你好aa",
		.output = L":好你好aa ",
		.cursorpos = 8,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, L'a' }, { 0, 0x300 }, { 0, L'a' }, { 0, L'b' }, { 0, L'c' }, { 0, L'd' }, { 0, L'e' }, { 0, L'f' }, { 0, L'g' }, { 0, L'h' }, { 0, L'i' }, { 1, KEY_LEFT } },
		.keypresscount = 12,
		.result = L"àabcdefghi",
		.output = L":abcdefghi",
		.cursorpos = 9,
	},
	{
		.keypresses = (struct keyspec[]) { { 0, 0x300 } },
		.keypresscount = 1,
		.result = L"",
		.output = L":         ",
		.cursorpos = 1,
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

	size_t bufferlength = wcslen(testdata[_i].output);
	wchar_t buffer[bufferlength + 1];
	int x, y;
	getyx(cmdline.window, y, x);
	mvwinnwstr(cmdline.window, 0, 0, buffer, bufferlength);
	ck_assert(wcscmp(buffer, testdata[_i].output) == 0);
	ck_assert_int_eq(x, testdata[_i].cursorpos);
	ck_assert_int_eq(y, 0);

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
