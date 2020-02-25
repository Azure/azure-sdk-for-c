// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_test.h>

#include <az_span.h>
#include <az_span_private.h>

#include <_az_cfg.h>

void test_mut_span()
{
  // swap
  {
    uint8_t a_array[] = "Hello world!";
    uint8_t b_array[] = "Goodbye!";
    az_span const a = AZ_SPAN_FROM_INITIALIZED_BUFFER(a_array);
    az_span const b = AZ_SPAN_FROM_INITIALIZED_BUFFER(b_array);
    _az_span_swap(a, b);
    TEST_ASSERT(az_span_is_equal(a, AZ_SPAN_FROM_STR("Goodbye!\0ld!\0")));
    TEST_ASSERT(az_span_is_equal(b, AZ_SPAN_FROM_STR("Hello wor")));
  }
  // swap an empty span
  {
    uint8_t a_array[] = "Hello world!";
    az_span const a = AZ_SPAN_FROM_INITIALIZED_BUFFER(a_array);
    az_span const b = { 0 };
    _az_span_swap(a, b);
    TEST_ASSERT(az_span_is_equal(a, AZ_SPAN_FROM_STR("Hello world!\0")));
    TEST_ASSERT(az_span_is_equal(b, AZ_SPAN_FROM_STR("")));
  }
}
