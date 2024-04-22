// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_test_log.h>
#include <azure/core/az_log.h>
#include <azure/core/az_result.h>
#include <azure/core/az_retry.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include <azure/core/_az_cfg.h>

static int _log_retry = 0;
static void _log_listener(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_RETRY:
      _log_retry++;
      assert_int_equal(az_span_size(message), 0);
      break;
    default:
      assert_true(false);
  }
}

static bool _should_write_iot_retry_only(az_log_classification classification)
{
  switch (classification)
  {
    case AZ_LOG_RETRY:
      return true;
    default:
      return false;
  }
}

static bool _should_write_nothing(az_log_classification classification)
{
  (void)classification;
  return false;
}

static void test_az_retry_calculate_delay_common_timings_success()
{
  assert_int_equal(2229, az_retry_calculate_delay(5, 1, 500, 100000, 1234));
  assert_int_equal(321, az_retry_calculate_delay(5000, 1, 500, 100000, 4321));

  // Operation already took more than the back-off interval.
  assert_int_equal(0, az_retry_calculate_delay(10000, 1, 500, 100000, 4321));

  // Max retry exceeded.
  assert_int_equal(9995, az_retry_calculate_delay(5, 5, 500, 10000, 4321));
}

static void test_az_retry_calculate_delay_overflow_time_success()
{
  assert_int_equal(
      0,
      az_retry_calculate_delay(
          INT32_MAX - 1, INT16_MAX - 1, INT32_MAX - 1, INT32_MAX - 1, INT32_MAX - 1));

  assert_int_equal(
      INT32_MAX - 1,
      az_retry_calculate_delay(0, INT16_MAX - 1, INT32_MAX - 1, INT32_MAX - 1, INT32_MAX - 1));
}

static void test_az_retry_calculate_delay_logging_succeed()
{
  az_log_set_message_callback(_log_listener);
  az_log_set_classification_filter_callback(_should_write_iot_retry_only);

  _log_retry = 0;
  assert_int_equal(2229, az_retry_calculate_delay(5, 1, 500, 100000, 1234));
  assert_int_equal(_az_BUILT_WITH_LOGGING(1, 0), _log_retry);

  az_log_set_message_callback(NULL);
  az_log_set_classification_filter_callback(NULL);
}

static void test_az_retry_calculate_delay_no_logging_succeed()
{
  az_log_set_message_callback(_log_listener);
  az_log_set_classification_filter_callback(_should_write_nothing);

  _log_retry = 0;
  assert_int_equal(2229, az_retry_calculate_delay(5, 1, 500, 100000, 1234));
  assert_int_equal(_az_BUILT_WITH_LOGGING(0, 0), _log_retry);

  az_log_set_message_callback(NULL);
  az_log_set_classification_filter_callback(NULL);
}

int test_az_retry()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_retry_calculate_delay_common_timings_success),
    cmocka_unit_test(test_az_retry_calculate_delay_overflow_time_success),
    cmocka_unit_test(test_az_retry_calculate_delay_logging_succeed),
    cmocka_unit_test(test_az_retry_calculate_delay_no_logging_succeed),
  };
  return cmocka_run_group_tests_name("az_core_retry", tests, NULL, NULL);
}
