/* See LICENSE file for copyright and license details. */
#include <check.h>
#include <stdlib.h>

#include "../src/listmodel.h"
#include "../src/listmodel_impl.h"
#include "../src/listview.h"
#include "../src/util.h"
#include "tests.h"

static struct testmodel {
	struct listmodel listmodel;
	size_t count;
} model;

static struct listview listview;

static size_t testmodel_count(struct listmodel *listmodel)
{
	return container_of(listmodel, struct testmodel, listmodel)->count;
}

static size_t testmodel_render(struct listmodel *listmodel, wchar_t *buffer, size_t len, size_t width, size_t index)
{
	ck_assert(len >= width);
	ck_assert(index < container_of(listmodel, struct testmodel, listmodel)->count);

	for(size_t i = 0; i < width; i++) {
		buffer[i] = L' ';
	}
	buffer[width] = L'\0';

	return width;
}

static void testmodel_init(struct testmodel *model, size_t count)
{
	listmodel_init(&model->listmodel);
	model->listmodel.count = testmodel_count;
	model->listmodel.render = testmodel_render;
	model->count = count;
}

static void testmodel_destroy(struct testmodel *model)
{
	listmodel_destroy(&model->listmodel);
}

static void testmodel_add(struct testmodel *model, size_t index)
{
	model->count++;
	listmodel_notify_change(&model->listmodel, MODEL_ADD, index, 0);
}

static void testmodel_remove(struct testmodel *model, size_t index)
{
	model->count--;
	listmodel_notify_change(&model->listmodel, MODEL_REMOVE, 0, index);
}

static void testmodel_change(struct testmodel *model, size_t newindex, size_t oldindex)
{
	listmodel_notify_change(&model->listmodel, MODEL_CHANGE, newindex, oldindex);
}

static bool create_view() {
	return listview_init(&listview, &model.listmodel, 0, 0, 80, 25);
}

START_TEST(test_listview_initialization)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	ck_assert_uint_eq(listview_getindex(&listview), 0);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


START_TEST(test_listview_setindex_first)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 0);

	ck_assert_uint_eq(listview_getindex(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_setindex_middle)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 10);
	ck_assert_uint_eq(listview_getindex(&listview), 10);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_setindex_last)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 99);
	ck_assert_uint_eq(listview_getindex(&listview), 99);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_setindex_outofbounds)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 100);
	ck_assert_uint_eq(listview_getindex(&listview), 99);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_setindex_scrollonlywhenneeded)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
	listview_setindex(&listview, 30);
	ck_assert_uint_eq(listview_getindex(&listview), 30);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);
	listview_setindex(&listview, 35);
	ck_assert_uint_eq(listview_getindex(&listview), 35);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


START_TEST(test_listview_up)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 10);
	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 9);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_up_onfirstindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_up_scroll)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_setindex(&listview, 16);
	ck_assert_uint_eq(listview_getindex(&listview), 16);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 15);
	ck_assert_uint_eq(listview_getfirst(&listview), 15);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


START_TEST(test_listview_down)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_down(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 1);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_down_onlastindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 99);
	listview_down(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 99);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_down_scroll)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_down(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 41);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


START_TEST(test_listview_pageup_onfirstindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_pageup_scroll)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 16);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


START_TEST(test_listview_pagedown_onlastindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 99);
	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 99);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_pagedown_scroll)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 24);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 49);
	ck_assert_uint_eq(listview_getfirst(&listview), 25);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_pageupdown_smalllist)
{
	testmodel_init(&model, 10);
	assert_oom(create_view() == true);

	listview_pagedown(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 9);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_pageup(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 0);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


START_TEST(test_listview_resize_makebigger)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 50);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_resize_makebiggerthanfirsttoendoflist)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 90);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 10);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_resize_makebiggerthanlist)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 120);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_resize_makesmaller)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_setindex(&listview, 30);
	listview_resize(&listview, 80, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 30);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_resize_makesmallerthanindextofirst)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 21);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_resize_toone)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 40);
	listview_resize(&listview, 80, 1);
	ck_assert_uint_eq(listview_getindex(&listview), 40);
	ck_assert_uint_eq(listview_getfirst(&listview), 40);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST


static void gotofirst16index25(void)
{
	listview_setindex(&listview, 40);
	listview_setindex(&listview, 25);
}

START_TEST(test_listview_modelchange_addbeforefirst)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_add(&model, 10);
	ck_assert_uint_eq(listview_getindex(&listview), 26);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_removebeforefirst)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_remove(&model, 10);
	ck_assert_uint_eq(listview_getindex(&listview), 24);
	ck_assert_uint_eq(listview_getfirst(&listview), 15);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_addbetweenfirstandindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_add(&model, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 26);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_removebetweenfirstandindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_remove(&model, 20);
	ck_assert_uint_eq(listview_getindex(&listview), 24);
	ck_assert_uint_eq(listview_getfirst(&listview), 15);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_addonindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_add(&model, 25);
	ck_assert_uint_eq(listview_getindex(&listview), 26);
	ck_assert_uint_eq(listview_getfirst(&listview), 17);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_removeonindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_remove(&model, 25);
	ck_assert_uint_eq(listview_getindex(&listview), 25);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_addafterindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_add(&model, 30);
	ck_assert_uint_eq(listview_getindex(&listview), 25);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_removeafterindex)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	gotofirst16index25();
	testmodel_remove(&model, 30);
	ck_assert_uint_eq(listview_getindex(&listview), 25);
	ck_assert_uint_eq(listview_getfirst(&listview), 16);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_removeafterindex_atendoflist)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 99);
	listview_up(&listview);
	ck_assert_uint_eq(listview_getindex(&listview), 98);
	ck_assert_uint_eq(listview_getfirst(&listview), 75);

	testmodel_remove(&model, 99);

	ck_assert_uint_eq(listview_getindex(&listview), 98);
	ck_assert_uint_eq(listview_getfirst(&listview), 74);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_moveditem_itemselected)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 50);
	ck_assert_uint_eq(listview_getindex(&listview), 50);
	ck_assert_uint_eq(listview_getfirst(&listview), 26);

	testmodel_change(&model, 30, 50);

	ck_assert_uint_eq(listview_getindex(&listview), 30);
	ck_assert_uint_eq(listview_getfirst(&listview), 26);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_moveditem_itemnotselected_addedbefore)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 50);
	ck_assert_uint_eq(listview_getindex(&listview), 50);
	ck_assert_uint_eq(listview_getfirst(&listview), 26);

	testmodel_change(&model, 40, 60);

	ck_assert_uint_eq(listview_getindex(&listview), 51);
	ck_assert_uint_eq(listview_getfirst(&listview), 27);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_moveditem_itemnotselected_removedbefore)
{
	testmodel_init(&model, 100);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 50);
	ck_assert_uint_eq(listview_getindex(&listview), 50);
	ck_assert_uint_eq(listview_getfirst(&listview), 26);

	testmodel_change(&model, 60, 40);

	ck_assert_uint_eq(listview_getindex(&listview), 49);
	ck_assert_uint_eq(listview_getfirst(&listview), 25);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_addbeforefirst_smallist)
{
	testmodel_init(&model, 10);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 5);
	ck_assert_uint_eq(listview_getindex(&listview), 5);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	testmodel_add(&model, 3);

	ck_assert_uint_eq(listview_getindex(&listview), 6);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

START_TEST(test_listview_modelchange_removelastindex_listitemsequalsrows)
{
	testmodel_init(&model, 25);
	assert_oom(create_view() == true);

	listview_setindex(&listview, 24);
	testmodel_remove(&model, 24);
	ck_assert_uint_eq(listview_getindex(&listview), 23);
	ck_assert_uint_eq(listview_getfirst(&listview), 0);

	listview_destroy(&listview);
	testmodel_destroy(&model);
}
END_TEST

Suite *listview_suite(void)
{
	Suite *suite;
	TCase *tcase;

	suite = suite_create("List view");

	tcase = tcase_create("Core");
	tcase_add_test(tcase, test_listview_initialization);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("setindex");
	tcase_add_test(tcase, test_listview_setindex_first);
	tcase_add_test(tcase, test_listview_setindex_middle);
	tcase_add_test(tcase, test_listview_setindex_last);
	tcase_add_test(tcase, test_listview_setindex_outofbounds);
	tcase_add_test(tcase, test_listview_setindex_scrollonlywhenneeded);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("up");
	tcase_add_test(tcase, test_listview_up);
	tcase_add_test(tcase, test_listview_up_onfirstindex);
	tcase_add_test(tcase, test_listview_up_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("down");
	tcase_add_test(tcase, test_listview_down);
	tcase_add_test(tcase, test_listview_down_onlastindex);
	tcase_add_test(tcase, test_listview_down_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("pageup");
	tcase_add_test(tcase, test_listview_pageup_onfirstindex);
	tcase_add_test(tcase, test_listview_pageup_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("pagedown");
	tcase_add_test(tcase, test_listview_pagedown_onlastindex);
	tcase_add_test(tcase, test_listview_pagedown_scroll);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("pageupdown_smalllist");
	tcase_add_test(tcase, test_listview_pageupdown_smalllist);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("resize");
	tcase_add_test(tcase, test_listview_resize_makebigger);
	tcase_add_test(tcase, test_listview_resize_makebiggerthanfirsttoendoflist);
	tcase_add_test(tcase, test_listview_resize_makebiggerthanlist);
	tcase_add_test(tcase, test_listview_resize_makesmaller);
	tcase_add_test(tcase, test_listview_resize_makesmallerthanindextofirst);
	tcase_add_test(tcase, test_listview_resize_toone);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("modelchange");
	tcase_add_test(tcase, test_listview_modelchange_addbeforefirst);
	tcase_add_test(tcase, test_listview_modelchange_removebeforefirst);
	tcase_add_test(tcase, test_listview_modelchange_addbetweenfirstandindex);
	tcase_add_test(tcase, test_listview_modelchange_removebetweenfirstandindex);
	tcase_add_test(tcase, test_listview_modelchange_addonindex);
	tcase_add_test(tcase, test_listview_modelchange_removeonindex);
	tcase_add_test(tcase, test_listview_modelchange_addafterindex);
	tcase_add_test(tcase, test_listview_modelchange_removeafterindex);
	tcase_add_test(tcase, test_listview_modelchange_removeafterindex_atendoflist);
	tcase_add_test(tcase, test_listview_modelchange_moveditem_itemselected);
	tcase_add_test(tcase, test_listview_modelchange_moveditem_itemnotselected_addedbefore);
	tcase_add_test(tcase, test_listview_modelchange_moveditem_itemnotselected_removedbefore);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("modelchange_smalllist");
	tcase_add_test(tcase, test_listview_modelchange_addbeforefirst_smallist);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("modelchange_listitemsequalsrows");
	tcase_add_test(tcase, test_listview_modelchange_removelastindex_listitemsequalsrows);
	suite_add_tcase(suite, tcase);

	return suite;
}

