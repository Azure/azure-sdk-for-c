// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_private.h"
#include "az_test_definitions.h"
#include <az_span.h>
#include <az_span_private.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

void test_az_span_replace(void** state)
{
  (void)state;
  {
    // Replace inside content with smaller content -> left shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("1X78");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 1, 6, AZ_SPAN_FROM_STR("X")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace inside content with smaller content at one position -> right shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("12X345678");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 2, 2, AZ_SPAN_FROM_STR("X")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace inside content with smaller content at one position at the end -> no shift required
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("1234567890");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 8, 8, AZ_SPAN_FROM_STR("90")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace all content with smaller content -> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("X");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 0, 8, AZ_SPAN_FROM_STR("X")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace all content with bigger content -> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("X12345678X");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 0, 8, AZ_SPAN_FROM_STR("X12345678X")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace content with smaller content at the beggining -> right shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("XXX12345678");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 0, 0, AZ_SPAN_FROM_STR("XXX")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace content with same size content size 1-> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1");
    az_span expected = AZ_SPAN_FROM_STR("2");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 200);

    assert_true(_az_span_replace(&builder, 0, 1, AZ_SPAN_FROM_STR("2")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace content with same size content size > 1-> no shift
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span expected = AZ_SPAN_FROM_STR("4321");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 4);

    assert_true(_az_span_replace(&builder, 0, 4, AZ_SPAN_FROM_STR("4321")) == AZ_OK);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Append another span after replacing -> builder should keep writing at the end
    uint8_t array[10];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span expected = AZ_SPAN_FROM_STR("1X34AB");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 10);

    TEST_EXPECT_SUCCESS(_az_span_replace(&builder, 1, 2, AZ_SPAN_FROM_STR("X")));

    builder = az_span_append(builder, AZ_SPAN_FROM_STR("AB"));
    assert_int_equal(
        az_span_capacity(builder) + az_span_length(AZ_SPAN_FROM_STR("AB"))
            + az_span_length(initial_state),
        10);

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Replace at last position -> insert at the end
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("123");
    az_span expected = AZ_SPAN_FROM_STR("1234");

    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 4);

    TEST_EXPECT_SUCCESS(_az_span_replace(&builder, 3, 3, AZ_SPAN_FROM_STR("4")));

    az_span const result = builder;
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Fail on buffer override -> try to replace with something bigger than buffer
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 4);

    assert_true(_az_span_replace(&builder, 0, 4, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    // Fail on builder empty -> try to replace content from empty builder
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    assert_true(_az_span_replace(&builder, 0, 1, AZ_SPAN_FROM_STR("2")) == AZ_ERROR_ARG);
  }
  {
    // Replace content on empty builder -> insert at the end
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    assert_true(_az_span_replace(&builder, 0, 0, AZ_SPAN_FROM_STR("2")) == AZ_OK);
    az_span const result = builder;
    az_span const expected = AZ_SPAN_FROM_STR("2");
    assert_true(az_span_is_content_equal(result, expected));
  }
  {
    // Fail if trying to replace out of bounds content -> start and end out
    uint8_t array[400];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 400);

    assert_true(_az_span_replace(&builder, 30, 31, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    // Fail when trying to replace out of bounds -> end position out
    uint8_t array[40];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 40);

    assert_true(_az_span_replace(&builder, 4, 5, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    // Fail when start is greater than end
    uint8_t array[40];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    builder = az_span_append(builder, initial_state);
    assert_int_equal(az_span_capacity(builder) + az_span_length(initial_state), 40);

    assert_true(_az_span_replace(&builder, 3, 1, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
}
