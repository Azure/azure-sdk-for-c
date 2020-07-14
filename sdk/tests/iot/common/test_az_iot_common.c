// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_common.h"
#include <azure/iot/az_iot_common.h>
#include <azure/iot/internal/az_iot_common_internal.h>
#include <azure/core/az_log.h>
#include <azure/core/az_precondition.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/az_span.h>
#include <az_test_log.h>
#include <az_test_precondition.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

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

static void test_az_iot_u64toa_size_success()
{
  assert_int_equal(_az_iot_u64toa_size(0), 1);
  assert_int_equal(_az_iot_u64toa_size(9), 1);
  assert_int_equal(_az_iot_u64toa_size(10), 2);
  assert_int_equal(_az_iot_u64toa_size(99), 2);
  assert_int_equal(_az_iot_u64toa_size(10000000000ul), 11);
  assert_int_equal(_az_iot_u64toa_size(19999999999ul), 11);
  assert_int_equal(_az_iot_u64toa_size(1000000000000000000ul), 19);
  assert_int_equal(_az_iot_u64toa_size(1999999999999999999ul), 19);
  assert_int_equal(_az_iot_u64toa_size(10000000000000000000ul), 20);
  assert_int_equal(_az_iot_u64toa_size(18446744073709551615ul), 20);
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

static int _log_retry = 0;
static void _log_listener(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_IOT_RETRY:
      _log_retry++;
      assert_ptr_equal(az_span_ptr(message), (void*)0);
      assert_int_equal(az_span_size(message), 0);
      break;
    default:
      assert_true(false);
  }
}

static void test_az_iot_provisioning_client_logging_succeed()
{
  az_log_classification const classifications[] = { AZ_LOG_IOT_RETRY, AZ_LOG_END_OF_LIST };
  az_log_set_classifications(classifications);
  az_log_set_callback(_log_listener);

  assert_int_equal(0, _log_retry);
  _log_retry = 0;
  assert_int_equal(2229, az_iot_retry_calc_delay(5, 1, 500, 100000, 1234));
  assert_int_equal(_az_BUILT_WITH_LOGGING(1, 0), _log_retry);

  az_log_set_callback(NULL);
  az_log_set_classifications(NULL);
}

static void test_az_span_copy_url_encode_succeed()
{
  az_span url_decoded_span = AZ_SPAN_FROM_STR("abc/=%012");

  uint8_t url_encoded[15];
  az_span url_encoded_span = AZ_SPAN_FROM_BUFFER(url_encoded);

  az_span remaining;

  uint8_t expected_result[] = "abc%2F%3D%25012";

  assert_int_equal(_az_span_copy_url_encode(url_encoded_span, url_decoded_span, &remaining), AZ_OK);
  assert_int_equal(az_span_size(remaining), 0);
  assert_int_equal(az_span_size(url_encoded_span) - az_span_size(remaining), _az_COUNTOF(expected_result) - 1);
}

static void test_az_span_copy_url_encode_insufficient_size_fail()
{
  az_span url_decoded_span = AZ_SPAN_FROM_STR("abc/=%012");

  uint8_t url_encoded[14]; // Needs 15 bytes, this will cause a failure (as expected by this test).
  az_span url_encoded_span = AZ_SPAN_FROM_BUFFER(url_encoded);

  az_span remaining;

  assert_int_equal(_az_span_copy_url_encode(url_encoded_span, url_decoded_span, &remaining), AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_common()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_u32toa_size_success),
    cmocka_unit_test(test_az_iot_u64toa_size_success),
    cmocka_unit_test(test_az_iot_is_success_status_translate_success),
    cmocka_unit_test(test_az_iot_is_retriable_status_translate_success),
    cmocka_unit_test(test_az_iot_retry_calc_delay_common_timings_success),
    cmocka_unit_test(test_az_iot_retry_calc_delay_overflow_time_success),
    cmocka_unit_test(test_az_iot_provisioning_client_logging_succeed),
    cmocka_unit_test(test_az_span_copy_url_encode_succeed),
    cmocka_unit_test(test_az_span_copy_url_encode_insufficient_size_fail),
  };
  return cmocka_run_group_tests_name("az_iot_common", tests, NULL, NULL);
}
