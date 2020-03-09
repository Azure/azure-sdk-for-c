// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_span_private.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void az_span_from_string_non_ascii_roundtrip()
{
  // Some non-ASCII characters (Chinese) in the middle of ascii text.
  // Explicitly escaping them to avoid changing file encoding in IDEs like VS.
  // Concatening strings together to be able to use escaped character sequence:
  // https://en.cppreference.com/w/cpp/language/string_literal
  az_span span = AZ_SPAN_FROM_STR("12"
                                  "\xE6"
                                  "\xB1"
                                  "\x89"
                                  "\xE5"
                                  "\xAD"
                                  "\x97"
                                  "345");
  assert_int_equal(11, az_span_length(span));

  char roundTripBuffer[20] = "abcdefghijklmnopq";

  assert_return_code(az_span_to_str(roundTripBuffer, 20, span), AZ_OK);

  assert_int_equal(11, az_span_length(span));

  // Verify only 11 bytes are overwritten
  // and the remaining characters are left as is
  assert_int_equal(0x31, roundTripBuffer[0]); // '1'
  assert_int_equal(0x32, roundTripBuffer[1]); // '2'
  assert_int_equal(0xffffffffffffffe6, roundTripBuffer[2]);
  assert_int_equal(0xffffffffffffffb1, roundTripBuffer[3]);
  assert_int_equal(0xffffffffffffff89, roundTripBuffer[4]);
  assert_int_equal(0xffffffffffffffe5, roundTripBuffer[5]);
  assert_int_equal(0xffffffffffffffad, roundTripBuffer[6]);
  assert_int_equal(0xffffffffffffff97, roundTripBuffer[7]);
  assert_int_equal(0x33, roundTripBuffer[8]); // '3'
  assert_int_equal(0x34, roundTripBuffer[9]); // '4'
  assert_int_equal(0x35, roundTripBuffer[10]); // '5'
  assert_int_equal(0, roundTripBuffer[11]);
  assert_int_equal(0x6d, roundTripBuffer[12]); // 'm'
  assert_int_equal(0x6e, roundTripBuffer[13]);
  assert_int_equal(0x6f, roundTripBuffer[14]);
  assert_int_equal(0x70, roundTripBuffer[15]);
  assert_int_equal(0x71, roundTripBuffer[16]); // 'q'
  assert_int_equal(0, roundTripBuffer[17]);
}

void az_span_from_string_non_ascii_latin_roundtrip()
{
  // Creating a span from a non-ASCII character: 'LATIN SMALL LETTER E WITH GRAVE'
  // Explicitly escaping them to avoid changing file encoding in IDEs like VS.
  az_span span = AZ_SPAN_FROM_STR("\xC3"
                                  "\xA8");
  assert_int_equal(2, az_span_length(span));

  char roundTripBuffer[3] = { 0, 0, 0 };

  assert_int_equal(0, roundTripBuffer[0]);
  assert_int_equal(0, roundTripBuffer[1]);
  assert_int_equal(0, roundTripBuffer[2]);

  assert_return_code(az_span_to_str(roundTripBuffer, 3, span), AZ_OK);

  assert_int_equal(2, az_span_length(span));
  assert_int_equal(0xffffffffffffffc3, roundTripBuffer[0]);
  assert_int_equal(0xffffffffffffffa8, roundTripBuffer[1]);
  assert_int_equal(0, roundTripBuffer[2]);
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

  az_span_from_string_non_ascii_roundtrip();
  az_span_from_string_non_ascii_latin_roundtrip();
}
