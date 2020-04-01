// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_test_span.h
 *
 * @brief Test only utilities for helping validate spans.
 */

#ifndef _az_SPAN_TESTING_H
#define _az_SPAN_TESTING_H

#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include <az_result.h>
#include <az_span.h>

#include <stdio.h>

#include <_az_cfg_prefix.h>


/**
 * @brief az_span_for_test_init returns a span over a byte buffer, directly invoking
 * #az_span_init.  The buffer is initialized with 0xFF to help tests check against buffer overflow.
 *
 * @param[in] ptr The memory address of the 1st byte in the byte buffer.
 * @param[in] length The number of bytes initialized in the byte buffer.
 * @param[in] capacity The number of total bytes in the byte buffer.
 * @return az_span The "view" over the byte buffer, with the buffer filled with 0xFF.
 */
AZ_NODISCARD AZ_INLINE az_span az_span_for_test_init(uint8_t* ptr, int32_t length, int32_t capacity)
{
  az_span new_span = az_span_init(ptr, length, capacity);
  az_span_set(new_span, 0xcc);
  return new_span;
}

/**
 * @brief az_span_for_test_verify verifies that the span has been correctly set during the test.
 * In addition to memcmp of expected/actual data, it also checks against buffer overflow.
 *
 * The function will assert on any unexpected results.
 *
 * @param[in] result_span Span that has the result of the test run.
 * @param[in] buffer_expected Buffer that contains expected results of the test and that result_span
 * will match on success.
 * @param[in] length_expected The expected length of result_span.
 * @param[in] capacity_expected The expected capacity of result_span.
 */
AZ_INLINE void az_span_for_test_verify(
    az_span result_span,
    const void* const buffer_expected,
    int32_t length_expected,
    int32_t capacity_expected)
{
  assert_int_equal(az_span_length(result_span), length_expected);
  assert_int_equal(az_span_capacity(result_span), capacity_expected);
  assert_memory_equal(az_span_ptr(result_span), (size_t)buffer_expected, (size_t)length_expected);

  for (int32_t i = az_span_length(result_span); i < az_span_capacity(result_span); i++)
  {
    assert_true(*(uint8_t*)(az_span_ptr(result_span) + i) == 0xcc);
  }
}

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_TESTING_H
