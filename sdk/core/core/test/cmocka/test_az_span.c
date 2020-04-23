// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <stdarg.h>
#include <stddef.h>

#include <az_span.h>

#include <limits.h>
#include <setjmp.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

static void test_az_span_getters(void** state)
{
  (void)state;

  uint8_t example[] = "example";
  az_span span = AZ_SPAN_FROM_BUFFER(example);
  assert_int_equal(az_span_size(span), 8);
  assert_ptr_equal(az_span_ptr(span), &example);
}

static void az_single_char_ascii_lower_test(void** state)
{
  (void)state;

  for (uint8_t i = 0; i <= SCHAR_MAX; ++i)
  {
    uint8_t buffer[1] = { i };
    az_span span = AZ_SPAN_FROM_BUFFER(buffer);

    // Comparison to itself should return true for all values in the range.
    assert_true(az_span_is_content_equal_ignoring_case(span, span));

    // For ASCII letters, verify that comparing upper and lower case return true.
    if (i >= 'A' && i <= 'Z')
    {
      uint8_t lower[1] = { (uint8_t)(i + 32) };
      az_span lowerSpan = AZ_SPAN_FROM_BUFFER(lower);
      assert_true(az_span_is_content_equal_ignoring_case(span, lowerSpan));
      assert_true(az_span_is_content_equal_ignoring_case(lowerSpan, span));
    }
    else if (i >= 'a' && i <= 'z')
    {
      uint8_t upper[1] = { (uint8_t)(i - 32) };
      az_span upperSpan = AZ_SPAN_FROM_BUFFER(upper);
      assert_true(az_span_is_content_equal_ignoring_case(span, upperSpan));
      assert_true(az_span_is_content_equal_ignoring_case(upperSpan, span));
    }
    else
    {
      // Make sure that no other comparison returns true.
      for (uint8_t j = 0; j <= SCHAR_MAX; ++j)
      {
        uint8_t other[1] = { j };
        az_span otherSpan = AZ_SPAN_FROM_BUFFER(other);

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

static void az_span_to_lower_test(void** state)
{
  (void)state;

  az_span a = AZ_SPAN_FROM_STR("one");
  az_span b = AZ_SPAN_FROM_STR("One");
  az_span c = AZ_SPAN_FROM_STR("ones");
  az_span d = AZ_SPAN_FROM_STR("ona");
  assert_true(az_span_is_content_equal_ignoring_case(a, b));
  assert_false(az_span_is_content_equal_ignoring_case(a, c));
  assert_false(az_span_is_content_equal_ignoring_case(a, d));
}

static void az_span_atou64_return_errors(void** state)
{
  (void)state;
  // sample span
  az_span sample = AZ_SPAN_FROM_STR("test");
  uint64_t out = 0;

  assert_true(az_span_atou64(sample, &out) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
}

static void az_span_atou32_test(void** state)
{
  (void)state;
  az_span number = AZ_SPAN_FROM_STR("1024");
  uint32_t value = 0;

  assert_return_code(az_span_atou32(number, &value), AZ_OK);
  assert_int_equal(value, 1024);
}

static void az_span_to_str_test(void** state)
{
  (void)state;
  az_span sample = AZ_SPAN_FROM_STR("hello World!");
  char str[20];

  az_span_to_str(str, 20, sample);
  assert_string_equal(str, "hello World!");
}

static void az_span_find_beginning_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefg");
  az_span target = AZ_SPAN_FROM_STR("abc");

  assert_int_equal(az_span_find(span, target), 0);
}

static void az_span_find_middle_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefg");
  az_span target = AZ_SPAN_FROM_STR("gab");

  assert_int_equal(az_span_find(span, target), 6);
}

static void az_span_find_end_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefgh");
  az_span target = AZ_SPAN_FROM_STR("efgh");

  assert_int_equal(az_span_find(span, target), 11);
}

static void az_span_find_source_target_identical_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefg");
  az_span target = AZ_SPAN_FROM_STR("abcdefgabcdefg");

  assert_int_equal(az_span_find(span, target), 0);
}

static void az_span_find_not_found_fail(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefg");
  az_span target = AZ_SPAN_FROM_STR("abd");

  assert_int_equal(az_span_find(span, target), -1);
}

static void az_span_find_error_cases_fail(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefgabcdefg");
  az_span target = AZ_SPAN_FROM_STR("abd");

  assert_int_equal(az_span_find(AZ_SPAN_NULL, AZ_SPAN_NULL), 0);
  assert_int_equal(az_span_find(span, AZ_SPAN_NULL), 0);
  assert_int_equal(az_span_find(AZ_SPAN_NULL, target), -1);
}

static void az_span_find_target_longer_than_source_fails(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("aa");
  az_span target = AZ_SPAN_FROM_STR("aaa");

  assert_int_equal(az_span_find(span, target), -1);
}

static void az_span_find_target_overlap_continuation_of_source_fails(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcd");
  az_span target = AZ_SPAN_FROM_STR("cde");

  assert_int_equal(az_span_find(span, target), -1);
}

static void az_span_find_target_more_chars_than_prefix_of_source_fails(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcd");
  az_span target = AZ_SPAN_FROM_STR("zab");

  assert_int_equal(az_span_find(span, target), -1);
}

static void az_span_find_overlapping_target_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefghij");
  az_span target = az_span_slice(span, 6, 9);

  assert_int_equal(az_span_find(span, target), 6);
}

static void az_span_find_embedded_NULLs_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcd\0\0fghij");
  az_span target = AZ_SPAN_FROM_STR("\0\0");

  assert_int_equal(az_span_find(span, target), 4);
}

static void az_span_find_capacity_checks_success(void** state)
{
  (void)state;

  uint8_t* buffer = (uint8_t*)"aaaa";

  assert_int_equal(az_span_find(az_span_init(buffer, 2), az_span_init(buffer, 2)), 0);
  assert_int_equal(az_span_find(az_span_init(buffer, 2), az_span_init(buffer, 0)), 0);
  assert_int_equal(az_span_find(az_span_init(buffer, 0), az_span_init(buffer, 0)), 0);

  assert_int_equal(az_span_find(az_span_init(buffer, 2), az_span_init(buffer + 1, 2)), 0);
  assert_int_equal(az_span_find(az_span_init(buffer + 1, 2), az_span_init(buffer, 2)), 0);
  assert_int_equal(az_span_find(az_span_init(buffer + 1, 2), az_span_init(buffer + 1, 2)), 0);
  assert_int_equal(az_span_find(az_span_init(buffer, 2), az_span_init(buffer + 2, 2)), 0);
  assert_int_equal(az_span_find(az_span_init(buffer + 2, 2), az_span_init(buffer, 2)), 0);
}

static void az_span_find_overlapping_checks_success(void** state)
{
  (void)state;

  az_span span = AZ_SPAN_FROM_STR("abcdefghij");
  az_span source = az_span_slice(span, 1, 4);
  az_span target = az_span_slice(span, 6, 9);
  assert_int_equal(az_span_find(source, target), -1);
  assert_int_equal(az_span_find(source, az_span_slice(span, 1, 5)), -1);
  assert_int_equal(az_span_find(source, az_span_slice(span, 2, 4)), 1);
}

static void test_az_span_replace(void** state)
{
  (void)state;
  {
    // Replace inside content with smaller content -> left shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("1X78");
    az_span remainder = az_span_copy(builder, initial_state);

    int32_t expected_remainder_size = 200 - az_span_size(initial_state);
    assert_int_equal(az_span_size(remainder), expected_remainder_size);

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 1, 6, AZ_SPAN_FROM_STR("X"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 4);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace inside content with smaller content at one position -> right shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("12X345678");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 200 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 2, 2, AZ_SPAN_FROM_STR("X"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 9);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace inside content with smaller content at one position at the end -> no shift required
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("1234567890");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 200 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 8, 8, AZ_SPAN_FROM_STR("90"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 10);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace all content with smaller content -> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("X");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 200 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 0, 8, AZ_SPAN_FROM_STR("X"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 1);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace all content with bigger content -> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("X12345678X");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 200 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 0, 8, AZ_SPAN_FROM_STR("X12345678X"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 10);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace content with smaller content at the beggining -> right shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("XXX12345678");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 200 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 0, 0, AZ_SPAN_FROM_STR("XXX"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 11);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace content with same size content size 1-> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1");
    az_span expected = AZ_SPAN_FROM_STR("2");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 200 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 0, 1, AZ_SPAN_FROM_STR("2"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 1);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace content with same size content size > 1-> no shift
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span expected = AZ_SPAN_FROM_STR("4321");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 4 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 0, 4, AZ_SPAN_FROM_STR("4321"))
        == AZ_OK);

    az_span const result = az_span_slice(builder, 0, 4);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Append another span after replacing -> builder should keep writing at the end
    uint8_t array[10];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span expected = AZ_SPAN_FROM_STR("1X34AB");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 10 - az_span_size(initial_state));

    TEST_EXPECT_SUCCESS(
        _az_span_replace(builder, az_span_size(initial_state), 1, 2, AZ_SPAN_FROM_STR("X")));

    remainder = az_span_copy(remainder, AZ_SPAN_FROM_STR("AB"));

    int32_t expected_remainder_size
        = 10 - (az_span_size(AZ_SPAN_FROM_STR("AB")) + az_span_size(initial_state));
    assert_int_equal(az_span_size(remainder), expected_remainder_size);

    az_span const result = az_span_slice(builder, 0, 6);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace at last position -> insert at the end
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("123");
    az_span expected = AZ_SPAN_FROM_STR("1234");

    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 4 - az_span_size(initial_state));

    TEST_EXPECT_SUCCESS(
        _az_span_replace(builder, az_span_size(initial_state), 3, 3, AZ_SPAN_FROM_STR("4")));

    az_span const result = az_span_slice(builder, 0, 4);
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Fail on buffer override -> try to replace with something bigger than buffer
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 4 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 0, 4, AZ_SPAN_FROM_STR("4321X"))
        == AZ_ERROR_ARG);
  }
  {
    // Fail on builder empty -> try to replace content from empty builder
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    assert_true(_az_span_replace(builder, 0, 0, 1, AZ_SPAN_FROM_STR("2")) == AZ_ERROR_ARG);
  }
  {
    // Replace content on empty builder -> insert at the end
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    assert_true(_az_span_replace(builder, 0, 0, 0, AZ_SPAN_FROM_STR("2")) == AZ_OK);
    az_span const result = az_span_slice(builder, 0, 1);
    az_span const expected = AZ_SPAN_FROM_STR("2");
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Fail if trying to replace out of bounds content -> start and end out
    uint8_t array[400];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 400 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 30, 31, AZ_SPAN_FROM_STR("4321X"))
        == AZ_ERROR_ARG);
  }
  {
    // Fail when trying to replace out of bounds -> end position out
    uint8_t array[40];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 40 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 4, 5, AZ_SPAN_FROM_STR("4321X"))
        == AZ_ERROR_ARG);
  }
  {
    // Fail when start is greater than end
    uint8_t array[40];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span remainder = az_span_copy(builder, initial_state);
    assert_int_equal(az_span_size(remainder), 40 - az_span_size(initial_state));

    assert_true(
        _az_span_replace(builder, az_span_size(initial_state), 3, 1, AZ_SPAN_FROM_STR("4321X"))
        == AZ_ERROR_ARG);
  }
}

static void az_span_i64toa_test(void** state)
{
  (void)state;
  uint8_t buffer[100];
  az_span b_span = AZ_SPAN_FROM_BUFFER(buffer);
  az_span remainder;
  int32_t size_before_write = az_span_size(b_span);
  int64_t number = 123;
  az_span number_str = AZ_SPAN_FROM_STR("123");

  assert_return_code(az_span_i64toa(b_span, number, &remainder), AZ_OK);
  assert_int_equal(size_before_write, az_span_size(b_span));
  // remainder should be size minus number of digits (3)
  assert_int_equal(az_span_size(remainder), size_before_write - 3);

  // create az_span for written data
  b_span = az_span_init(az_span_ptr(b_span), az_span_size(b_span) - az_span_size(remainder));

  assert_true(az_span_is_content_equal(b_span, number_str));

  // convert back
  uint64_t reverse = 0;
  assert_return_code(az_span_atou64(b_span, &reverse), AZ_OK);
  assert_int_equal(reverse, number);
}

static void az_span_i64toa_negative_number_test(void** state)
{
  (void)state;
  uint8_t buffer[100];
  az_span b_span = AZ_SPAN_FROM_BUFFER(buffer);
  az_span remainder;
  int32_t size_before_write = az_span_size(b_span);
  int64_t number = -123;
  az_span number_str = AZ_SPAN_FROM_STR("-123");

  assert_return_code(az_span_i64toa(b_span, number, &remainder), AZ_OK);
  assert_int_equal(size_before_write, az_span_size(b_span));
  // remainder should be size minus number of digits (4)
  assert_int_equal(az_span_size(remainder), size_before_write - 4);

  // create az_span for written data
  b_span = az_span_init(az_span_ptr(b_span), az_span_size(b_span) - az_span_size(remainder));

  assert_true(az_span_is_content_equal(b_span, number_str));

  // convert back TODO: az_span_ato64 should support negative numbers since az_span_i64toa support
  // it. https://github.com/Azure/azure-sdk-for-c/issues/598
  /* uint64_t reverse = 0;
  assert_return_code(az_span_atou64(b_span, &reverse), AZ_OK);
  assert_int_equal(reverse, number);
  */
}

static void az_span_slice_to_end_test(void** state)
{
  (void)state;
  uint8_t raw_buffer[20];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  assert_int_equal(az_span_size(buffer), 20);

  az_span result = az_span_slice_to_end(buffer, 1);
  assert_int_equal(az_span_size(result), 19);

  result = az_span_slice_to_end(buffer, 5);
  assert_int_equal(az_span_size(result), 15);
}

static void az_span_test_macro_only_allows_byte_buffers(void** state)
{
  (void)state;
  {
    uint8_t uint8_buffer[2];
    assert_int_equal(_az_IS_ARRAY(uint8_buffer), 1);
    assert_int_equal(_az_IS_BYTE_ARRAY(uint8_buffer), 1);
    az_span valid = AZ_SPAN_FROM_BUFFER(uint8_buffer);
    assert_int_equal(az_span_size(valid), 2);
  }

  {
    char char_buffer[2];
    assert_int_equal(_az_IS_ARRAY(char_buffer), 1);
    assert_int_equal(_az_IS_BYTE_ARRAY(char_buffer), 1);
    az_span valid = AZ_SPAN_FROM_BUFFER(char_buffer);
    assert_int_equal(az_span_size(valid), 2);
  }

  {
    uint32_t uint32_buffer[2];
    assert_int_equal(_az_IS_ARRAY(uint32_buffer), 1);
    assert_int_equal(_az_IS_BYTE_ARRAY(uint32_buffer), 0);
  }

  {
    uint8_t x = 1;
    uint8_t* p1 = &x;
    assert_int_equal(_az_IS_ARRAY(p1), 0);
    assert_int_equal(_az_IS_BYTE_ARRAY(p1), 0);
  }

  {
    char* p1 = "HELLO";
    assert_int_equal(_az_IS_ARRAY(p1), 0);
    assert_int_equal(_az_IS_BYTE_ARRAY(p1), 0);
  }
}

static void az_span_from_str_succeeds(void** state)
{
  (void)state;
  char* str = "HelloWorld";
  az_span buffer = az_span_from_str(str);

  assert_int_equal(az_span_size(buffer), 10);
  assert_true(az_span_ptr(buffer) != NULL);
  assert_true((char*)az_span_ptr(buffer) == str);
}

static void az_span_copy_uint8_succeeds(void** state)
{
  (void)state;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);

  buffer = az_span_copy_u8(buffer, 'a');
  assert_int_equal(az_span_size(buffer), 14);
  buffer = az_span_copy_u8(buffer, 'b');
  assert_int_equal(az_span_size(buffer), 13);
  buffer = az_span_copy_u8(buffer, 'c');
  assert_int_equal(az_span_size(buffer), 12);

  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 3), AZ_SPAN_FROM_STR("abc")));
}

static void az_span_i32toa_succeeds(void** state)
{
  (void)state;
  int32_t v = 12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_i32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 10);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 5), AZ_SPAN_FROM_STR("12345")));
}

static void az_span_i32toa_negative_succeeds(void** state)
{
  (void)state;
  int32_t v = -12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_i32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 9);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 6), AZ_SPAN_FROM_STR("-12345")));
}

static void az_span_i32toa_zero_succeeds(void** state)
{
  (void)state;
  int32_t v = 0;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_i32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 14);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 1), AZ_SPAN_FROM_STR("0")));
}

static void az_span_i32toa_max_int_succeeds(void** state)
{
  (void)state;
  int32_t v = 2147483647;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_i32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 5);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 10), AZ_SPAN_FROM_STR("2147483647")));
}

static void az_span_i32toa_overflow_fails(void** state)
{
  (void)state;
  int32_t v = 2147483647;
  uint8_t raw_buffer[4];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_span_i32toa(buffer, v, &out_span) == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void az_span_u32toa_succeeds(void** state)
{
  (void)state;
  uint32_t v = 12345;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_u32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 10);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 5), AZ_SPAN_FROM_STR("12345")));
}

static void az_span_u32toa_zero_succeeds(void** state)
{
  (void)state;
  uint32_t v = 0;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_u32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 14);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 1), AZ_SPAN_FROM_STR("0")));
}

static void az_span_u32toa_max_uint_succeeds(void** state)
{
  (void)state;
  uint32_t v = 4294967295;
  uint8_t raw_buffer[15];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_succeeded(az_span_u32toa(buffer, v, &out_span)));
  assert_int_equal(az_span_size(out_span), 5);
  assert_true(az_span_is_content_equal(
      az_span_slice(AZ_SPAN_FROM_BUFFER(raw_buffer), 0, 10), AZ_SPAN_FROM_STR("4294967295")));
}

static void az_span_u32toa_overflow_fails(void** state)
{
  (void)state;
  uint32_t v = 2147483647;
  uint8_t raw_buffer[4];
  az_span buffer = AZ_SPAN_FROM_BUFFER(raw_buffer);
  az_span out_span;

  assert_true(az_span_u32toa(buffer, v, &out_span) == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

int test_az_span()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(az_span_slice_to_end_test),
    cmocka_unit_test(test_az_span_getters),
    cmocka_unit_test(az_single_char_ascii_lower_test),
    cmocka_unit_test(az_span_to_lower_test),
    cmocka_unit_test(az_span_to_str_test),
    cmocka_unit_test(az_span_find_beginning_success),
    cmocka_unit_test(az_span_find_middle_success),
    cmocka_unit_test(az_span_find_end_success),
    cmocka_unit_test(az_span_find_source_target_identical_success),
    cmocka_unit_test(az_span_find_not_found_fail),
    cmocka_unit_test(az_span_find_error_cases_fail),
    cmocka_unit_test(az_span_find_target_longer_than_source_fails),
    cmocka_unit_test(az_span_find_target_overlap_continuation_of_source_fails),
    cmocka_unit_test(az_span_find_target_more_chars_than_prefix_of_source_fails),
    cmocka_unit_test(az_span_find_overlapping_target_success),
    cmocka_unit_test(az_span_find_embedded_NULLs_success),
    cmocka_unit_test(az_span_find_capacity_checks_success),
    cmocka_unit_test(az_span_find_overlapping_checks_success),
    cmocka_unit_test(test_az_span_replace),
    cmocka_unit_test(az_span_atou64_return_errors),
    cmocka_unit_test(az_span_atou32_test),
    cmocka_unit_test(az_span_i64toa_negative_number_test),
    cmocka_unit_test(az_span_i64toa_test),
    cmocka_unit_test(az_span_test_macro_only_allows_byte_buffers),
    cmocka_unit_test(az_span_from_str_succeeds),
    cmocka_unit_test(az_span_copy_uint8_succeeds),
    cmocka_unit_test(az_span_i32toa_succeeds),
    cmocka_unit_test(az_span_i32toa_negative_succeeds),
    cmocka_unit_test(az_span_i32toa_max_int_succeeds),
    cmocka_unit_test(az_span_i32toa_zero_succeeds),
    cmocka_unit_test(az_span_i32toa_overflow_fails),
    cmocka_unit_test(az_span_u32toa_succeeds),
    cmocka_unit_test(az_span_u32toa_zero_succeeds),
    cmocka_unit_test(az_span_u32toa_max_uint_succeeds),
    cmocka_unit_test(az_span_u32toa_overflow_fails),
  };
  return cmocka_run_group_tests_name("az_core_span", tests, NULL, NULL);
}
