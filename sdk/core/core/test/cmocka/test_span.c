// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_span_private.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void az_span_from_string_non_ascii()
{
  az_span span = AZ_SPAN_FROM_STR("12汉字345");
  assert_int_equal(11, az_span_length(span));
}

void az_span_from_string_non_ascii_latin()
{
  az_span span = AZ_SPAN_FROM_STR("è");
  assert_int_equal(2, az_span_length(span));
}

void az_span_from_string_non_ascii_roundtrip()
{
  az_span span = AZ_SPAN_FROM_STR("12汉字345");
  char label2[20] = "abcdefghijklmnopq";
  char* labelPtr = label2;

  assert_int_equal(AZ_OK, az_span_to_str(labelPtr, 20, span));
  assert_int_equal(0x31, label2[0]); // '1'
  assert_int_equal(0x32, label2[1]); // '2'
  assert_int_equal(0x3F, label2[2]); // '?' - this is unexpected
  assert_int_equal(0x3F, label2[3]); // '?' - this is unexpected
  assert_int_equal(0x33, label2[4]); // '3'
  assert_int_equal(0x34, label2[5]); // '4'
  assert_int_equal(0x35, label2[6]); // '5'
  assert_int_equal(0, label2[7]);
  assert_int_equal(105, label2[8]); // 'i'
  assert_int_equal(106, label2[9]);
  assert_int_equal(107, label2[10]);
  assert_int_equal(108, label2[11]);
  assert_int_equal(109, label2[12]);
  assert_int_equal(110, label2[13]);
  assert_int_equal(111, label2[14]);
  assert_int_equal(112, label2[15]);
  assert_int_equal(113, label2[16]); // 'q'
  assert_int_equal(0, label2[17]);
}

void az_span_from_string_non_ascii_latin_roundtrip()
{
  az_span span = AZ_SPAN_FROM_STR("è");
  char arr[3] = "";

  assert_int_equal(0, arr[0]);
  assert_int_equal(0, arr[1]);
  assert_int_equal(0, arr[2]);

  assert_int_equal(AZ_OK, az_span_to_str(arr, 3, span));

  assert_int_equal(0xC3, arr[0]);
  assert_int_equal(0xA8, arr[1]);
  assert_int_equal(0, arr[2]);
}

void az_span_append_uint8_NULL_out_span_fails()
{
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  assert_true(az_span_append_uint8(buffer, 'a', NULL) == AZ_ERROR_ARG);
}

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

  assert_true(az_span_append_i32toa(buffer, v, &out_span) == AZ_ERROR_BUFFER_OVERFLOW);
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

  assert_true(az_span_append_u32toa(buffer, v, &out_span) == AZ_ERROR_BUFFER_OVERFLOW);
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

  az_span_append_uint8_NULL_out_span_fails();
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
