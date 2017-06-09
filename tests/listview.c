/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <stdlib.h>

#include "../src/listmodel.h"
#include "../src/listmodel_impl.h"
#include "../src/listview.h"

static struct listmodel model;
static struct listview listview;

static size_t testmodel_count(struct listmodel *model)
{
	return (size_t)model->data;
}

static size_t testmodel_render(struct listmodel *model, wchar_t *buffer, size_t len, size_t width, size_t index)
{
	ck_assert(len >= width);
	ck_assert(index < (size_t)model->data);

	for(size_t i = 0; i < width; i++) {
		buffer[i] = L' ';
	}
	buffer[width] = L'\0';

	return width;
}

static void testmodel_init(struct listmodel *model, size_t count)
{
	listmodel_init(model);
	model->count = testmodel_count;
	model->render = testmodel_render;
	model->data = (void*)count;
}

static void testmodel_add(struct listmodel *model, size_t index)
{
	model->data = (void*)((size_t)model->data + 1);
	listmodel_notify_change(model, index, MODEL_ADD);
}

static void testmodel_remove(struct listmodel *model, size_t index)
{
	model->data = (void*)((size_t)model->data - 1);
	listmodel_notify_change(model, index, MODEL_REMOVE);
}

static void setup(void)
{
	testmodel_init(&model, 100);
	listview_init(&listview, &model, 0, 0, 80, 25);
}

static void setup_smalllist(void)
{
	testmodel_init(&model, 10);
	listview_init(&listview, &model, 0, 0, 80, 25);
}

static void setup_listitemsequalsrows(void)
{
	testmodel_init(&model, 25);
	listview_init(&listview, &model, 0, 0, 80, 25);
}

static void teardown(void)
{
	listview_free(&listview);
	listmodel_free(&model);
}

START_TEST(test_listview_initialization)
{
	ck_assert_uint_eq(listview_getindex(&listview), 0);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);
}
END_TEST


START_TEST(test_listview_setindex_first)
{
	listview_setindex(&listview, 0);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
}
END_TEST

START_TEST(test_listview_setindex_middle)
{
	listview_setindex(&listview, 10);
	ck_assert_uint_eq(listview_getindex(&listview), 10);
}
END_TEST

START_TEST(test_listview_setindex_last)
{
	listview_setindex(&listview, 99);
	ck_assert_uint_eq(listview_getindex(&listview), 99);
}
END_TEST

START_TEST(test_listview_setindex_outofbounds)
{
	listview_setindex(&listview, 100);
	ck_assert_uint_eq(listview_getindex(&listview), 99);
}
END_TEST

START_TEST(test_listview_setindex_scrollonlywhenneeded)
{
	listview_setindex(&listview, 40);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
	listview_setindex(&listview, 30);
	ck_assert_uint_eq(listview_getindex(&listview), 30);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
	listview_setindex(&listview, 35);
	ck_assert_uint_eq(listview_getindex(&listview), 35);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
}
END_TEST


START_TEST(test_listview_up)
{
	listview_setindex(&listview, 10);
	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 9);
}
END_TEST

START_TEST(test_listview_up_onfirstindex)
{
	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
}
END_TEST

START_TEST(test_listview_up_scroll)
{
	listview_setindex(&listview, 40);
	listview_setindex(&listview, 16);
	ck_assert_uint_eq(listview_getindex(&listview), 16);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 15);
	ck_assert_uint_eq(listview_getfirst(&listview), 15);
}
END_TEST


START_TEST(test_listview_down)
{
	listview_down(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 1);
}
END_TEST

START_TEST(test_listview_down_onlastindex)
{
	listview_setindex(&listview, 99);
	listview_down(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 99);
}
END_TEST

START_TEST(test_listview_down_scroll)
{
	listview_setindex(&listview, 40);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_down(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 41);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);
}
END_TEST


START_TEST(test_listview_pageup_onfirstindex)
{
	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
}
END_TEST

START_TEST(test_listview_pageup_scroll)
{
	listview_setindex(&listview, 40);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 16);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);
}
END_TEST


START_TEST(test_listview_pagedown_onlastindex)
{
	listview_setindex(&listview, 99);
	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 99);
}
END_TEST

START_TEST(test_listview_pagedown_scroll)
{
	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 24);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 49);
	ck_assert_uint_eq(listview_getfirst(&listview), 25);
}
END_TEST

START_TEST(test_listview_pageupdown_smalllist)
{
	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 9);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);
}
END_TEST


START_TEST(test_listview_resize_makebigger)
{
	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 50);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
}
END_TEST

START_TEST(test_listview_resize_makebiggerthanfirsttoendoflist)
{
	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 90);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 10);
}
END_TEST

START_TEST(test_listview_resize_makebiggerthanlist)
{
	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 120);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);
}
END_TEST

START_TEST(test_listview_resize_makesmaller)
{
	listview_setindex(&listview, 40);
	listview_setindex(&listview, 30);
	listview_resize(&listview, 80, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 30);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
}
END_TEST

START_TEST(test_listview_resize_makesmallerthanindextofirst)
{
	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 21);
}
END_TEST

START_TEST(test_listview_resize_toone)
{
	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 1);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 40);
}
END_TEST


static void gotofirst16index25(void)
{
	listview_setindex(&listview, 40);
	listview_setindex(&listview, 25);
}

START_TEST(test_listview_modelchange_addbeforefirst)
{
	gotofirst16index25();
	testmodel_add(&model, 10);
	ck_assert_uint_eq(listview_getindex(&listview), 26);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);
}
END_TEST

START_TEST(test_listview_modelchange_removebeforefirst)
{
	gotofirst16index25();
	testmodel_remove(&model, 10);
	ck_assert_uint_eq(listview_getindex(&listview), 24);
	ck_assert_uint_eq(listview_getfirst(&listview), 15);
}
END_TEST

START_TEST(test_listview_modelchange_addbetweenfirstandindex)
{
	gotofirst16index25();
	testmodel_add(&model, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 26);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);
}
END_TEST

START_TEST(test_listview_modelchange_removebetweenfirstandindex)
{
	gotofirst16index25();
	testmodel_remove(&model, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 24);
	ck_assert_uint_eq(listview_getfirst(&listview), 15);
}
END_TEST

START_TEST(test_listview_modelchange_addonindex)
{
	gotofirst16index25();
	testmodel_add(&model, 25);
	ck_assert_uint_eq(listview_getindex(&listview), 26);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);
}
END_TEST

START_TEST(test_listview_modelchange_removeonindex)
{
	gotofirst16index25();
	testmodel_remove(&model, 25);
	ck_assert_uint_eq(listview_getindex(&listview), 25);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
}
END_TEST

START_TEST(test_listview_modelchange_addafterindex)
{
	gotofirst16index25();
	testmodel_add(&model, 30);
	ck_assert_uint_eq(listview_getindex(&listview), 25);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
}
END_TEST

START_TEST(test_listview_modelchange_removeafterindex)
{
	gotofirst16index25();
	testmodel_remove(&model, 30);
	ck_assert_uint_eq(listview_getindex(&listview), 25);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
}
END_TEST

START_TEST(test_listview_modelchange_removeafterindex_atendoflist)
{
	listview_setindex(&listview, 99);
	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 98);
	ck_assert_uint_eq(listview_getfirst(&listview), 75);

	testmodel_remove(&model, 99);

	ck_assert_uint_eq(listview_getindex(&listview), 98);
	ck_assert_uint_eq(listview_getfirst(&listview), 74);
}
END_TEST

START_TEST(test_listview_modelchange_addbeforefirst_smallist)
{
	listview_setindex(&listview, 5);
	ck_assert_uint_eq(listview_getindex(&listview), 5);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	testmodel_add(&model, 3);

	ck_assert_uint_eq(listview_getindex(&listview), 6);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);
}
END_TEST

START_TEST(test_listview_modelchange_removelastindex_listitemsequalsrows)
{
	listview_setindex(&listview, 24);
	testmodel_remove(&model, 24);
	ck_assert_uint_eq(listview_getindex(&listview), 23);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);
}
END_TEST

Suite *listview_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("List view");

	tcase = tcase_create("Core");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_initialization);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("setindex");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_setindex_first);
	tcase_add_test(tcase, test_listview_setindex_middle);
	tcase_add_test(tcase, test_listview_setindex_last);
	tcase_add_test(tcase, test_listview_setindex_outofbounds);
	tcase_add_test(tcase, test_listview_setindex_scrollonlywhenneeded);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("up");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_up);
	tcase_add_test(tcase, test_listview_up_onfirstindex);
	tcase_add_test(tcase, test_listview_up_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("down");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_down);
	tcase_add_test(tcase, test_listview_down_onlastindex);
	tcase_add_test(tcase, test_listview_down_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("pageup");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_pageup_onfirstindex);
	tcase_add_test(tcase, test_listview_pageup_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("pagedown");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_pagedown_onlastindex);
	tcase_add_test(tcase, test_listview_pagedown_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("pageupdown_smalllist");
	tcase_add_checked_fixture(tcase, setup_smalllist, teardown);
	tcase_add_test(tcase, test_listview_pageupdown_smalllist);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("resize");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_resize_makebigger);
	tcase_add_test(tcase, test_listview_resize_makebiggerthanfirsttoendoflist);
	tcase_add_test(tcase, test_listview_resize_makebiggerthanlist);
	tcase_add_test(tcase, test_listview_resize_makesmaller);
	tcase_add_test(tcase, test_listview_resize_makesmallerthanindextofirst);
	tcase_add_test(tcase, test_listview_resize_toone);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("modelchange");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_listview_modelchange_addbeforefirst);
	tcase_add_test(tcase, test_listview_modelchange_removebeforefirst);
	tcase_add_test(tcase, test_listview_modelchange_addbetweenfirstandindex);
	tcase_add_test(tcase, test_listview_modelchange_removebetweenfirstandindex);
	tcase_add_test(tcase, test_listview_modelchange_addonindex);
	tcase_add_test(tcase, test_listview_modelchange_removeonindex);
	tcase_add_test(tcase, test_listview_modelchange_addafterindex);
	tcase_add_test(tcase, test_listview_modelchange_removeafterindex);
	tcase_add_test(tcase, test_listview_modelchange_removeafterindex_atendoflist);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("modelchange_smalllist");
	tcase_add_checked_fixture(tcase, setup_smalllist, teardown);
	tcase_add_test(tcase, test_listview_modelchange_addbeforefirst_smallist);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("modelchange_listitemsequalsrows");
	tcase_add_checked_fixture(tcase, setup_listitemsequalsrows, teardown);
	tcase_add_test(tcase, test_listview_modelchange_removelastindex_listitemsequalsrows);
	suite_add_tcase(suite, tcase);

	return suite;
}

