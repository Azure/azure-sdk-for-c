// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_core.h"
#include <az_iot_core.h>
#include <az_span.h>

#include <az_precondition_internal.h>
#include <az_precondition.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <az_test_precondition.h>

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
  assert_true(az_span_length(token) == 0);
  assert_true(az_span_ptr(out_span) == (az_span_ptr(span) + az_span_length(delim)));
  assert_true(az_span_length(out_span) == (az_span_length(span) - az_span_length(delim)));
  assert_true(az_span_capacity(out_span) == (az_span_capacity(span) - az_span_capacity(delim)));

  // token: "defg" (span+3)
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_ptr(token) == az_span_ptr(span));
  assert_true(az_span_length(token) == 4);
  assert_true(az_span_capacity(token) == az_span_capacity(span));
  assert_true(az_span_ptr(out_span) == (az_span_ptr(span) + az_span_length(token) + az_span_length(delim)));
  assert_true(az_span_length(out_span) == (az_span_length(span) - az_span_length(token) - az_span_length(delim)));
  assert_true(az_span_capacity(out_span) == (az_span_capacity(span) - az_span_length(token) - az_span_length(delim)));

  // token: "defg" (span+10)
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_ptr(token) == az_span_ptr(span));
  assert_true(az_span_length(token) == 4);
  assert_true(az_span_capacity(token) == az_span_length(span));
  assert_true(az_span_ptr(out_span) == (az_span_ptr(span) + az_span_length(token) + az_span_length(delim)));
  assert_true(az_span_length(out_span) == (az_span_length(span) - az_span_length(token) - az_span_length(delim)));
  assert_true(az_span_capacity(out_span) == (az_span_capacity(span) - az_span_length(token) - az_span_length(delim)));

  // token: "defg" (span+17)
  span = out_span;

  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_ptr(token) == az_span_ptr(span));
  assert_true(az_span_length(token) == 4);
  assert_true(az_span_capacity(token) == az_span_length(span));
  assert_true(az_span_ptr(out_span) == NULL);
  assert_true(az_span_length(out_span) == 0);
  assert_true(az_span_capacity(out_span) == 0);

  // Out_span is empty.
  span = out_span;
  
  token = az_span_token(span, delim, &out_span);
  assert_true(az_span_is_content_equal(token, AZ_SPAN_NULL));
}

int test_az_iot_core()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(az_span_token_success)
  };
  return cmocka_run_group_tests_name("az_iot_core", tests, NULL, NULL);
}
