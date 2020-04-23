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

static void az_span_token_success(void** state)
{
  (void)state;
  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefgabcdefg");
  az_span delim = AZ_SPAN_FROM_STR("abc");
  az_span token;
  az_span out_span;

  // token: ""
  token = az_span_token(span, delim, &out_span);
  assert_non_null(az_span_ptr(token));
  assert_true(az_span_size(token) == 0);
  assert_true(az_span_ptr(out_span) == (az_span_ptr(span) + az_span_size(delim)));
  assert_true(az_span_size(out_span) == (az_span_size(span) - az_span_size(delim)));

  // token: "defg" (span+3)
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_ptr(token) == az_span_ptr(span));
  assert_int_equal(az_span_size(token), 4);
  assert_true(
      az_span_ptr(out_span) == (az_span_ptr(span) + az_span_size(token) + az_span_size(delim)));
  assert_true(
      az_span_size(out_span) == (az_span_size(span) - az_span_size(token) - az_span_size(delim)));

  // token: "defg" (span+10)
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_ptr(token) == az_span_ptr(span));
  assert_int_equal(az_span_size(token), 4);
  assert_true(
      az_span_ptr(out_span) == (az_span_ptr(span) + az_span_size(token) + az_span_size(delim)));
  assert_true(
      az_span_size(out_span) == (az_span_size(span) - az_span_size(token) - az_span_size(delim)));

  // token: "defg" (span+17)
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_ptr(token) == az_span_ptr(span));
  assert_int_equal(az_span_size(token), 4);
  assert_true(az_span_ptr(out_span) == NULL);
  assert_true(az_span_size(out_span) == 0);

  // Out_span is empty.
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_is_content_equal(token, AZ_SPAN_NULL));
}

static void test_az_iot_get_status_from_uint32(void** state)
{
  (void)state;

  az_iot_status status;

  assert_int_equal(az_iot_get_status_from_uint32(200, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_OK, status);

  assert_int_equal(az_iot_get_status_from_uint32(202, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_ACCEPTED, status);

  assert_int_equal(az_iot_get_status_from_uint32(204, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_NO_CONTENT, status);

  assert_int_equal(az_iot_get_status_from_uint32(400, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_BAD_REQUEST, status);

  assert_int_equal(az_iot_get_status_from_uint32(401, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_UNAUTHORIZED, status);

  assert_int_equal(az_iot_get_status_from_uint32(403, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_FORBIDDEN, status);

  assert_int_equal(az_iot_get_status_from_uint32(404, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_NOT_FOUND, status);

  assert_int_equal(az_iot_get_status_from_uint32(405, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_NOT_ALLOWED, status);

  assert_int_equal(az_iot_get_status_from_uint32(409, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_NOT_CONFLICT, status);

  assert_int_equal(az_iot_get_status_from_uint32(412, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_PRECONDITION_FAILED, status);

  assert_int_equal(az_iot_get_status_from_uint32(413, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_REQUEST_TOO_LARGE, status);

  assert_int_equal(az_iot_get_status_from_uint32(415, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_UNSUPPORTED_TYPE, status);

  assert_int_equal(az_iot_get_status_from_uint32(429, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_THROTTLED, status);

  assert_int_equal(az_iot_get_status_from_uint32(499, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_CLIENT_CLOSED, status);

  assert_int_equal(az_iot_get_status_from_uint32(500, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_SERVER_ERROR, status);

  assert_int_equal(az_iot_get_status_from_uint32(502, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_BAD_GATEWAY, status);

  assert_int_equal(az_iot_get_status_from_uint32(503, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_SERVICE_UNAVAILABLE, status);

  assert_int_equal(az_iot_get_status_from_uint32(504, &status), AZ_OK);
  assert_int_equal(AZ_IOT_STATUS_TIMEOUT, status);

  assert_int_equal(az_iot_get_status_from_uint32(999, &status), AZ_ERROR_ITEM_NOT_FOUND);
}

int test_az_iot_core()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(az_span_token_success),
    cmocka_unit_test(test_az_iot_get_status_from_uint32),
  };
  return cmocka_run_group_tests_name("az_iot_core", tests, NULL, NULL);
}
