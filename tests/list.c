/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <stdlib.h>

#include "../src/list.h"
#include "tests.h"

struct list *list;

static void setup(void)
{
	list = list_new(0);
}

static void teardown(void)
{
	list_delete(list, NULL);
}

START_TEST(test_list_new)
{
	assert_oom(list != NULL);

	ck_assert_int_eq(list_length(list), 0);
}
END_TEST

START_TEST(test_list_deletenull)
{
	/* test should not crash */
	list_delete(NULL, NULL);
}
END_TEST

START_TEST(test_list_add_items)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 8192; i++)
		assert_oom(list_append(list, (void*)i) == true);

	ck_assert_int_eq(list_length(list), 8192);

	for(size_t i = 0; i < 8192; i++)
		ck_assert_uint_eq((size_t)list_get_item(list, i), i);
}
END_TEST

START_TEST(test_list_set_items)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 8192; i++)
		assert_oom(list_append(list, 0) == true);

	ck_assert_int_eq(list_length(list), 8192);

	for(size_t i = 0; i < 8192; i++)
		ck_assert_uint_eq((size_t)list_get_item(list, i), 0);

	for(size_t i = 0; i < 8192; i++)
		list_set_item(list, i, (void*)i);

	for(size_t i = 0; i < 8192; i++)
		ck_assert_uint_eq((size_t)list_get_item(list, i), i);
}
END_TEST

START_TEST(test_list_remove_items)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 4; i++)
		assert_oom(list_append(list, (void*)i) == true);

	ck_assert_int_eq(list_length(list), 4);

	list_remove(list, 0);
	list_remove(list, 0);

	ck_assert_int_eq(list_length(list), 2);

	for(size_t i = 0; i < 2; i++)
		ck_assert_uint_eq((size_t)list_get_item(list, i), i + 2);
}
END_TEST

static int compare_int(const void *item1, const void *item2)
{
	intptr_t val1 = *(intptr_t*)item1;
	intptr_t val2 = *(intptr_t*)item2;

	if(val1 < val2)
		return -1;
	else if(val1 > val2)
		return 1;
	return 0;
}

static int int_list_is_sorted(struct list *list)
{
	intptr_t last = (intptr_t)list_get_item(list, 0);
	for(size_t i = 1; i < 8192; i++) {
		intptr_t current = (intptr_t)list_get_item(list, i);
		if(last > current)
			return 0;
		last = current;
	}
	return 1;
}

START_TEST(test_list_sort)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 8192; i++) {
		intptr_t value = rand();
		assert_oom(list_append(list, (void*)value) == true);
	}

	list_sort(list, compare_int);
	ck_assert_int_eq(list_length(list), 8192);
	ck_assert_int_eq(int_list_is_sorted(list), 1);
}
END_TEST

START_TEST(test_list_insert_sort)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 8192; i++) {
		intptr_t value = rand();
		size_t insert_point;
		list_find_item_or_insertpoint(list, compare_int, (void*)value, &insert_point);
		assert_oom(list_insert(list, insert_point, (void*)value) == true);;
	}
	ck_assert_int_eq(list_length(list), 8192);
	ck_assert_int_eq(int_list_is_sorted(list), 1);
}
END_TEST

START_TEST(test_list_move_item)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 10; i++)
		assert_oom(list_append(list, (void*)i) == true);

	list_move_item(list, 4, 8);

	ck_assert_uint_eq((size_t)list_get_item(list, 0), 0);
	ck_assert_uint_eq((size_t)list_get_item(list, 1), 1);
	ck_assert_uint_eq((size_t)list_get_item(list, 2), 2);
	ck_assert_uint_eq((size_t)list_get_item(list, 3), 3);
	ck_assert_uint_eq((size_t)list_get_item(list, 4), 5);
	ck_assert_uint_eq((size_t)list_get_item(list, 5), 6);
	ck_assert_uint_eq((size_t)list_get_item(list, 6), 7);
	ck_assert_uint_eq((size_t)list_get_item(list, 7), 8);
	ck_assert_uint_eq((size_t)list_get_item(list, 8), 4);
	ck_assert_uint_eq((size_t)list_get_item(list, 9), 9);
}
END_TEST

START_TEST(test_list_move_item_backwards)
{
	assert_oom(list != NULL);

	for(size_t i = 0; i < 10; i++)
		assert_oom(list_append(list, (void*)i) == true);

	list_move_item(list, 7, 5);

	ck_assert_uint_eq((size_t)list_get_item(list, 0), 0);
	ck_assert_uint_eq((size_t)list_get_item(list, 1), 1);
	ck_assert_uint_eq((size_t)list_get_item(list, 2), 2);
	ck_assert_uint_eq((size_t)list_get_item(list, 3), 3);
	ck_assert_uint_eq((size_t)list_get_item(list, 4), 4);
	ck_assert_uint_eq((size_t)list_get_item(list, 5), 7);
	ck_assert_uint_eq((size_t)list_get_item(list, 6), 5);
	ck_assert_uint_eq((size_t)list_get_item(list, 7), 6);
	ck_assert_uint_eq((size_t)list_get_item(list, 8), 8);
	ck_assert_uint_eq((size_t)list_get_item(list, 9), 9);
}
END_TEST

Suite *list_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("List");

	tcase = tcase_create("Core");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_list_new);
	tcase_add_test(tcase, test_list_deletenull);
	tcase_add_test(tcase, test_list_add_items);
	tcase_add_test(tcase, test_list_set_items);
	tcase_add_test(tcase, test_list_remove_items);
	tcase_add_test(tcase, test_list_sort);
	tcase_add_test(tcase, test_list_insert_sort);
	tcase_add_test(tcase, test_list_move_item);
	tcase_add_test(tcase, test_list_move_item_backwards);
	suite_add_tcase(suite, tcase);

	return suite;
}
