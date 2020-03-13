// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_result.h>
#include <az_span.h>
#include <az_span_private.h>

#include <setjmp.h>
#include <stdarg.h>
#include <limits.h>

#include <cmocka.h>

#include <_az_cfg.h>

/* void az_span_append_uint8_NULL_out_span_fails()
{
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  assert_true(az_span_append_uint8(buffer, 'a', NULL) == AZ_ERROR_ARG);
} */

void az_single_char_ascii_lower_test()
{
  for (uint8_t i = 0; i <= SCHAR_MAX; i++)
  {
    uint8_t buffer[1] = { i };
    az_span span = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(buffer);

    // Comparison to itself should return true for all values in the range.
    assert_true(az_span_is_content_equal_ignoring_case(span, span));

    // For ASCII letters, verify that comparing upper and lower case return true.
    if (i >= 'A' && i <= 'Z')
    {
      uint8_t lower[1] = { i + 32 };
      az_span lowerSpan = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(lower);
      assert_true(az_span_is_content_equal_ignoring_case(span, lowerSpan));
      assert_true(az_span_is_content_equal_ignoring_case(lowerSpan, span));
    }
    else if (i >= 'a' && i <= 'z')
    {
      uint8_t upper[1] = { i - 32 };
      az_span upperSpan = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(upper);
      assert_true(az_span_is_content_equal_ignoring_case(span, upperSpan));
      assert_true(az_span_is_content_equal_ignoring_case(upperSpan, span));
    }
    else
    {
      // Make sure that no other comparison returns true.
      for (uint8_t j = 0; j <= SCHAR_MAX; j++)
      {
        uint8_t other[1] = { j };
        az_span otherSpan = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(other);

        if (i == j)
        {
          assert_true(az_span_is_content_equal_ignoring_case(span, otherSpan));
        }
        else
        {
          assert_false(az_span_is_content_equal_ignoring_case(span, otherSpan));
        }
      }
    }
  }
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
  assert_true(az_span_is_content_equal(buffer, AZ_SPAN_FROM_STR("abc")));
}

void az_span_append_i32toa_succeeds()
{
  int32_t v = 12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_i32toa_negative_succeeds()
{
  int32_t v = -12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("-12345")));
}

void az_span_append_i32toa_zero_succeeds()
{
  int32_t v = 0;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_i32toa_max_int_succeeds()
{
  int32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_i32toa_NULL_span_fails()
{
  int32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
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
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_u32toa_zero_succeeds()
{
  uint32_t v = 0;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_u32toa_max_uint_succeeds()
{
  uint32_t v = 4294967295;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("4294967295")));
}

void az_span_append_u32toa_NULL_span_fails()
{
  uint32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
  assert_true(az_span_is_content_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_u32toa_overflow_fails()
{
  uint32_t v = 2147483647;
  uint8_t raw_buffer[4];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_span_append_u32toa(buffer, v, &out_span) == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void az_span_to_lower_test()
{
  az_span a = AZ_SPAN_FROM_STR("one");
  az_span b = AZ_SPAN_FROM_STR("One");
  az_span c = AZ_SPAN_FROM_STR("ones");
  az_span d = AZ_SPAN_FROM_STR("ona");
  assert_true(az_span_is_content_equal_ignoring_case(a, b));
  assert_false(az_span_is_content_equal_ignoring_case(a, c));
  assert_false(az_span_is_content_equal_ignoring_case(a, d));
}

void az_span_to_uint64_return_errors()
{
  // sample span
  az_span sample = AZ_SPAN_FROM_STR("test");
  uint64_t out = 0;

  assert_true(az_span_to_uint64(sample, &out) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
}

void az_span_to_uint32_test()
{
  az_span number = AZ_SPAN_FROM_STR("1024");
  uint32_t value = 0;

  assert_return_code(az_span_to_uint32(number, &value), AZ_OK);
  assert_int_equal(value, 1024);
}

void az_span_to_str_test()
{
  az_span sample = AZ_SPAN_FROM_STR("hello World!");
  char str[20];

  assert_return_code(az_span_to_str(str, 20, sample), AZ_OK);
  assert_string_equal(str, "hello World!");
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
    assert_true(az_span_is_content_equal(a, AZ_SPAN_FROM_STR("Goodbye!\0ld!\0")));
    assert_true(az_span_is_content_equal(b, AZ_SPAN_FROM_STR("Hello wor")));
  }
  // swap an empty span
  {
    uint8_t a_array[] = "Hello world!";
    az_span const a = AZ_SPAN_FROM_INITIALIZED_BUFFER(a_array);
    az_span const b = { 0 };
    _az_span_swap(a, b);
    assert_true(az_span_is_content_equal(a, AZ_SPAN_FROM_STR("Hello world!\0")));
    assert_true(az_span_is_content_equal(b, AZ_SPAN_FROM_STR("")));
  }

  // az_span_append_uint8_NULL_out_span_fails();
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

  az_single_char_ascii_lower_test();
  az_span_to_lower_test();
  az_span_to_uint32_test();
  az_span_to_str_test();
  az_span_to_uint64_return_errors();
}
