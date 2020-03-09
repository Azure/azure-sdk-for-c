// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_span_private.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

/* void az_span_append_uint8_NULL_out_span_fails()
{
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  assert_true(az_span_append_uint8(buffer, 'a', NULL) == AZ_ERROR_ARG);
} */

void az_span_append_uint8_overflow_fails()
{
  uint8_t raw_buffer[2];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  assert_true(az_succeeded(az_span_append_uint8(buffer, 'a', &buffer)));
  assert_true(az_succeeded(az_span_append_uint8(buffer, 'b', &buffer)));
  assert_true(az_failed(az_span_append_uint8(buffer, 'c', &buffer)));
}

void az_span_append_uint8_succeeds()
{
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  assert_true(az_succeeded(az_span_append_uint8(buffer, 'a', &buffer)));
  assert_true(az_succeeded(az_span_append_uint8(buffer, 'b', &buffer)));
  assert_true(az_succeeded(az_span_append_uint8(buffer, 'c', &buffer)));
  assert_true(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("abc")));
}

void az_span_append_i32toa_succeeds()
{
  int32_t v = 12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_i32toa_negative_succeeds()
{
  int32_t v = -12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("-12345")));
}

void az_span_append_i32toa_zero_succeeds()
{
  int32_t v = 0;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_i32toa_max_int_succeeds()
{
  int32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_i32toa_NULL_span_fails()
{
  int32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_i32toa_overflow_fails()
{
  int32_t v = 2147483647;
  uint8_t raw_buffer[4];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_span_append_i32toa(buffer, v, &out_span) == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void az_span_append_u32toa_succeeds()
{
  uint32_t v = 12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_u32toa_zero_succeeds()
{
  uint32_t v = 0;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_u32toa_max_uint_succeeds()
{
  uint32_t v = 4294967295;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("4294967295")));
}

void az_span_append_u32toa_NULL_span_fails()
{
  uint32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_u32toa_overflow_fails()
{
  uint32_t v = 2147483647;
  uint8_t raw_buffer[4];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_span_append_u32toa(buffer, v, &out_span) == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_span(void** state)
{
  (void)state;
  // swap
  {
    uint8_t a_array[] = "Hello world!";
    uint8_t b_array[] = "Goodbye!";
    az_span const a = AZ_SPAN_FROM_INITIALIZED_BUFFER(a_array);
    az_span const b = AZ_SPAN_FROM_INITIALIZED_BUFFER(b_array);
    _az_span_swap(a, b);
    assert_true(az_span_is_equal(a, AZ_SPAN_FROM_STR("Goodbye!\0ld!\0")));
    assert_true(az_span_is_equal(b, AZ_SPAN_FROM_STR("Hello wor")));
  }
  // swap an empty span
  {
    uint8_t a_array[] = "Hello world!";
    az_span const a = AZ_SPAN_FROM_INITIALIZED_BUFFER(a_array);
    az_span const b = { 0 };
    _az_span_swap(a, b);
    assert_true(az_span_is_equal(a, AZ_SPAN_FROM_STR("Hello world!\0")));
    assert_true(az_span_is_equal(b, AZ_SPAN_FROM_STR("")));
  }

  //az_span_append_uint8_NULL_out_span_fails();
  az_span_append_uint8_overflow_fails();
  az_span_append_uint8_succeeds();

  az_span_append_i32toa_succeeds();
  az_span_append_i32toa_negative_succeeds();
  az_span_append_i32toa_max_int_succeeds();
  az_span_append_i32toa_zero_succeeds();
  az_span_append_i32toa_NULL_span_fails();
  az_span_append_i32toa_overflow_fails();

  az_span_append_u32toa_succeeds();
  az_span_append_u32toa_zero_succeeds();
  az_span_append_u32toa_max_uint_succeeds();
  az_span_append_u32toa_NULL_span_fails();
  az_span_append_u32toa_overflow_fails();
}
