// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_core.h"
#include <az_iot_core.h>
#include <az_span.h>

#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

static void test_az_iot_u32toa_size_success()
{
  assert_int_equal(_az_iot_u32toa_size(0), 1);
  assert_int_equal(_az_iot_u32toa_size(9), 1);
  assert_int_equal(_az_iot_u32toa_size(10), 2);
  assert_int_equal(_az_iot_u32toa_size(99), 2);
  assert_int_equal(_az_iot_u32toa_size(100), 3);
  assert_int_equal(_az_iot_u32toa_size(199), 3);
  assert_int_equal(_az_iot_u32toa_size(1000), 4);
  assert_int_equal(_az_iot_u32toa_size(1999), 4);
  assert_int_equal(_az_iot_u32toa_size(10000), 5);
  assert_int_equal(_az_iot_u32toa_size(19999), 5);
  assert_int_equal(_az_iot_u32toa_size(100000), 6);
  assert_int_equal(_az_iot_u32toa_size(199999), 6);
  assert_int_equal(_az_iot_u32toa_size(1000000), 7);
  assert_int_equal(_az_iot_u32toa_size(1999999), 7);
  assert_int_equal(_az_iot_u32toa_size(10000000), 8);
  assert_int_equal(_az_iot_u32toa_size(19999999), 8);
  assert_int_equal(_az_iot_u32toa_size(100000000), 9);
  assert_int_equal(_az_iot_u32toa_size(199999999), 9);
  assert_int_equal(_az_iot_u32toa_size(1000000000), 10);
  assert_int_equal(_az_iot_u32toa_size(4294967295), 10);
}

static void test_az_iot_is_success_status_translate_success()
{
  assert_true(az_iot_is_success_status(AZ_IOT_STATUS_OK));
  assert_true(az_iot_is_success_status(AZ_IOT_STATUS_NO_CONTENT));
  assert_true(az_iot_is_success_status(0));
  assert_true(az_iot_is_success_status(350));

  assert_false(az_iot_is_success_status(AZ_IOT_STATUS_BAD_REQUEST));
  assert_false(az_iot_is_success_status(AZ_IOT_STATUS_TIMEOUT));
  assert_false(az_iot_is_success_status(600));
}

static void test_az_iot_is_retriable_status_translate_success()
{
  assert_true(az_iot_is_retriable_status(AZ_IOT_STATUS_THROTTLED));
  assert_true(az_iot_is_retriable_status(AZ_IOT_STATUS_SERVER_ERROR));

  assert_false(az_iot_is_retriable_status(AZ_IOT_STATUS_OK));
  assert_false(az_iot_is_retriable_status(AZ_IOT_STATUS_UNAUTHORIZED));
}

static void test_az_iot_retry_calc_delay_common_timings_success()
{
  assert_int_equal(2229, az_iot_retry_calc_delay(5, 1, 500, 100000, 1234));
  assert_int_equal(321, az_iot_retry_calc_delay(5000, 1, 500, 100000, 4321));

  // Operation already took more than the back-off interval.
  assert_int_equal(0, az_iot_retry_calc_delay(10000, 1, 500, 100000, 4321));

  // Max retry exceeded.
  assert_int_equal(9995, az_iot_retry_calc_delay(5, 5, 500, 10000, 4321));
}

static void test_az_iot_retry_calc_delay_overflow_time_success()
{
  assert_int_equal(
      0,
      az_iot_retry_calc_delay(
          INT32_MAX - 1, INT16_MAX - 1, INT32_MAX - 1, INT32_MAX - 1, INT32_MAX - 1));

  assert_int_equal(
      INT32_MAX - 1,
      az_iot_retry_calc_delay(0, INT16_MAX - 1, INT32_MAX - 1, INT32_MAX - 1, INT32_MAX - 1));
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_core()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_u32toa_size_success),
    cmocka_unit_test(test_az_iot_is_success_status_translate_success),
    cmocka_unit_test(test_az_iot_is_retriable_status_translate_success),
    cmocka_unit_test(test_az_iot_retry_calc_delay_common_timings_success),
    cmocka_unit_test(test_az_iot_retry_calc_delay_overflow_time_success),
  };
  return cmocka_run_group_tests_name("az_iot_core", tests, NULL, NULL);
}
