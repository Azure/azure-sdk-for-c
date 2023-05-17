// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_test_precondition.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define RANDOM_TRIES 100
#define TEST_MILLISECOND_TIME 10

#ifndef __APPLE__

static int test_timer_callback_counter = 0;

// Will record the time at which the callback was called
static void _test_az_platform_timer_callback_set_time(void* callback_context)
{
  test_timer_callback_counter++;
  assert_int_equal(az_platform_clock_msec((int64_t*)callback_context), AZ_OK);
}

#endif // __APPLE__

// Checking for preconditions
#ifndef AZ_NO_PRECONDITION_CHECKING

ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_platform_clock_msec_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_clock_msec(NULL));
}

static void test_az_platform_get_random_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_get_random(NULL));
}

#ifndef __APPLE__

static void test_az_platform_timer_create_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_timer_create(NULL, NULL, NULL));
}

static void test_az_platform_timer_start_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_timer_start(NULL, 0));
}

static void test_az_platform_timer_destroy_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_timer_destroy(NULL));
}

static void test_az_platform_mutex_init_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_mutex_init(NULL));
}

static void test_az_platform_mutex_acquire_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_mutex_acquire(NULL));
}

static void test_az_platform_mutex_release_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_mutex_release(NULL));
}

static void test_az_platform_mutex_destroy_null(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_platform_mutex_destroy(NULL));
}

#endif // __APPLE__
#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_platform_clock_msec_once(void** state)
{
  (void)state;

  int64_t test_clock = 0;

  assert_int_equal(az_platform_clock_msec(&test_clock), AZ_OK);
  assert_true(test_clock >= 0);
}

static void test_az_platform_sleep_msec(void** state)
{
  (void)state;

  int64_t test_clock_one = 0; // Time before sleep.
  int64_t test_clock_two = 0; // Time after sleep.

  assert_int_equal(az_platform_clock_msec(&test_clock_one), AZ_OK);
  assert_int_equal(az_platform_sleep_msec(TEST_MILLISECOND_TIME), AZ_OK);
  assert_int_equal(az_platform_clock_msec(&test_clock_two), AZ_OK);

  assert_int_not_equal(test_clock_one, 0);
  assert_int_not_equal(test_clock_two, 0);
  assert_true((test_clock_two - test_clock_one) >= TEST_MILLISECOND_TIME);
}

static void test_az_platform_get_random(void** state)
{
  (void)state;

  int32_t test_random_1 = 0;
  int32_t test_random_2 = 0;
  assert_int_equal(az_platform_get_random(&test_random_1), AZ_OK);
  assert_int_equal(az_platform_get_random(&test_random_2), AZ_OK);
  assert_true(test_random_1 > -1);
  assert_true(test_random_2 > -1);

  int i = 0;
  while ((test_random_1 == test_random_2) && i < RANDOM_TRIES)
  {
    assert_int_equal(az_platform_get_random(&test_random_2), AZ_OK);
    assert_true(test_random_2 > -1);
    i++;
  }
  assert_true(test_random_2 != test_random_1);
}

#ifndef __APPLE__

static void test_az_platform_timer_single(void** state)
{
  (void)state;

  _az_platform_timer test_out_timer;
  int64_t test_clock_one = 0; // Time before timer start.
  int64_t test_clock_two = 0; // Time after timer callback.
  test_timer_callback_counter = 0;

  assert_int_equal(
      az_platform_timer_create(
          &test_out_timer, _test_az_platform_timer_callback_set_time, &test_clock_two),
      AZ_OK);
  assert_int_equal(az_platform_clock_msec(&test_clock_one), AZ_OK);
  assert_int_equal(az_platform_timer_start(&test_out_timer, TEST_MILLISECOND_TIME), AZ_OK);

  // Sleep until callback is triggered
  assert_int_equal(az_platform_sleep_msec(2 * TEST_MILLISECOND_TIME), AZ_OK);

  assert_int_equal(test_timer_callback_counter, 1);
  assert_int_not_equal(test_clock_one, 0);
  assert_int_not_equal(test_clock_two, 0);
  assert_true((test_clock_two - test_clock_one) >= TEST_MILLISECOND_TIME);

  assert_int_equal(az_platform_timer_destroy(&test_out_timer), AZ_OK);
}

static void test_az_platform_timer_double(void** state)
{
  (void)state;
  _az_platform_timer test_out_timer_1;
  _az_platform_timer test_out_timer_2;
  int64_t test_clock_one = 0;
  int64_t test_clock_two = 0;
  test_timer_callback_counter = 0;

  assert_int_equal(
      az_platform_timer_create(
          &test_out_timer_1, _test_az_platform_timer_callback_set_time, &test_clock_one),
      AZ_OK);
  assert_int_equal(
      az_platform_timer_create(
          &test_out_timer_2, _test_az_platform_timer_callback_set_time, &test_clock_two),
      AZ_OK);

  assert_int_equal(az_platform_timer_start(&test_out_timer_1, TEST_MILLISECOND_TIME), AZ_OK);
  assert_int_equal(az_platform_timer_start(&test_out_timer_2, 2 * TEST_MILLISECOND_TIME), AZ_OK);

  assert_int_equal(az_platform_sleep_msec(3 * TEST_MILLISECOND_TIME), AZ_OK);

  assert_int_equal(test_timer_callback_counter, 2);
  assert_int_not_equal(test_clock_one, 0);
  assert_int_not_equal(test_clock_two, 0);
  assert_true(test_clock_two > test_clock_one);

  assert_int_equal(az_platform_timer_destroy(&test_out_timer_1), AZ_OK);
  assert_int_equal(az_platform_timer_destroy(&test_out_timer_2), AZ_OK);
}

static void test_az_platform_mutex(void** state)
{
  (void)state;

  az_platform_mutex test_mutex_handle;

  assert_int_equal(az_platform_mutex_init(&test_mutex_handle), AZ_OK);
  assert_int_equal(az_platform_mutex_acquire(&test_mutex_handle), AZ_OK);
  assert_int_equal(az_platform_mutex_release(&test_mutex_handle), AZ_OK);
  assert_int_equal(az_platform_mutex_destroy(&test_mutex_handle), AZ_OK);
}

static void test_az_platform_mutex_double(void** state)
{
  (void)state;

  az_platform_mutex test_mutex_handle_1;
  az_platform_mutex test_mutex_handle_2;

  assert_int_equal(az_platform_mutex_init(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_init(&test_mutex_handle_2), AZ_OK);
  assert_int_equal(az_platform_mutex_acquire(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_acquire(&test_mutex_handle_2), AZ_OK);
  assert_int_equal(az_platform_mutex_release(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_release(&test_mutex_handle_2), AZ_OK);
  assert_int_equal(az_platform_mutex_destroy(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_destroy(&test_mutex_handle_2), AZ_OK);
}

static void test_az_platform_mutex_reentrant(void** state)
{
  (void)state;

  az_platform_mutex test_mutex_handle_1;

  assert_int_equal(az_platform_mutex_init(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_acquire(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_acquire(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_release(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_release(&test_mutex_handle_1), AZ_OK);
  assert_int_equal(az_platform_mutex_destroy(&test_mutex_handle_1), AZ_OK);
}

#endif // __APPLE__

int test_az_platform()
{
  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_platform_clock_msec_null),
    cmocka_unit_test(test_az_platform_get_random_null),
#ifndef __APPLE__
    cmocka_unit_test(test_az_platform_timer_create_null),
    cmocka_unit_test(test_az_platform_timer_start_null),
    cmocka_unit_test(test_az_platform_timer_destroy_null),
    cmocka_unit_test(test_az_platform_mutex_init_null),
    cmocka_unit_test(test_az_platform_mutex_acquire_null),
    cmocka_unit_test(test_az_platform_mutex_release_null),
    cmocka_unit_test(test_az_platform_mutex_destroy_null),
#endif // __APPLE__
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_platform_clock_msec_once),
    cmocka_unit_test(test_az_platform_sleep_msec),
    cmocka_unit_test(test_az_platform_get_random),
#ifndef __APPLE__
    cmocka_unit_test(test_az_platform_timer_single),
    cmocka_unit_test(test_az_platform_timer_double),
    cmocka_unit_test(test_az_platform_mutex),
    cmocka_unit_test(test_az_platform_mutex_double),
    cmocka_unit_test(test_az_platform_mutex_reentrant),
#endif // __APPLE__
  };
  return cmocka_run_group_tests_name("az_platform", tests, NULL, NULL);
}