#include <check.h>
#include <stdlib.h>

#include "../src/listmodel.h"
#include "../src/listmodel_impl.h"

static struct listmodel model;

static size_t cb_index;
static enum model_change cb_change;
static void *cb_data;
static unsigned int cb_count;

static void change_callback(size_t index, enum model_change change, void *data)
{
	cb_count++;
	cb_index = index;
	cb_change = change;
	cb_data = data;
}

static void setup(void)
{
	cb_count = 0;
	cb_index = 0;
	cb_change = MODEL_RELOAD;
	cb_data = NULL;
	listmodel_init(&model);
}

static void teardown(void)
{
	listmodel_free(&model);
}

START_TEST(test_single_callback)
{
	listmodel_register_change_callback(&model, change_callback, &model);
	ck_assert_uint_eq(cb_count, 0);
	ck_assert_uint_eq(cb_index, 0);
	ck_assert_ptr_eq(cb_data, NULL);
	ck_assert_int_eq(cb_change, MODEL_RELOAD);

	listmodel_notify_change(&model, 0x1337, MODEL_ADD);
	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_index, 0x1337);
	ck_assert_ptr_eq(cb_data, &model);
	ck_assert_int_eq(cb_change, MODEL_ADD);

	listmodel_notify_change(&model, 0xdeadbeef, MODEL_REMOVE);
	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_index, 0xdeadbeef);
	ck_assert_ptr_eq(cb_data, &model);
	ck_assert_int_eq(cb_change, MODEL_REMOVE);
}
END_TEST

START_TEST(test_two_callbacks)
{
	listmodel_register_change_callback(&model, change_callback, &model);
	listmodel_register_change_callback(&model, change_callback, &cb_count);

	listmodel_notify_change(&model, 0x1337, MODEL_ADD);
	ck_assert_uint_eq(cb_count, 2);
	ck_assert_uint_eq(cb_index, 0x1337);
	ck_assert_ptr_eq(cb_data, &cb_count);
	ck_assert_int_eq(cb_change, MODEL_ADD);
}
END_TEST

START_TEST(test_unregister_callback)
{
	listmodel_register_change_callback(&model, change_callback, &model);

	listmodel_unregister_change_callback(&model, change_callback, NULL);

	listmodel_notify_change(&model, 0x1337, MODEL_ADD);
	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_index, 0x1337);
	ck_assert_ptr_eq(cb_data, &model);
	ck_assert_int_eq(cb_change, MODEL_ADD);

	listmodel_unregister_change_callback(&model, change_callback, &model);

	listmodel_notify_change(&model, 0xdeadbeef, MODEL_REMOVE);
	ck_assert_uint_eq(cb_count, 1);
	ck_assert_uint_eq(cb_index, 0x1337);
	ck_assert_ptr_eq(cb_data, &model);
	ck_assert_int_eq(cb_change, MODEL_ADD);
}
END_TEST

Suite *listmodel_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("List model");

	tcase = tcase_create("Callback");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_single_callback);
	tcase_add_test(tcase, test_two_callbacks);
	tcase_add_test(tcase, test_unregister_callback);
	suite_add_tcase(suite, tcase);

	return suite;
}
