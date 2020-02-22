// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_span_private.h>

#include <az_test.h>

#include <_az_cfg.h>

void test_span_builder_replace()
{
  {
    // Replace inside content with smaller content -> left shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("1X78");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 1, 6, AZ_SPAN_FROM_STR("X")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace inside content with smaller content at one position -> right shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("12X345678");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 2, 2, AZ_SPAN_FROM_STR("X")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace inside content with smaller content at one position at the end -> no shift required
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("1234567890");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 8, 8, AZ_SPAN_FROM_STR("90")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace all content with smaller content -> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("X");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 0, 8, AZ_SPAN_FROM_STR("X")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace all content with bigger content -> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("X12345678X");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 0, 8, AZ_SPAN_FROM_STR("X12345678X")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace content with smaller content at the beggining -> right shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("12345678");
    az_span expected = AZ_SPAN_FROM_STR("XXX12345678");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 0, 0, AZ_SPAN_FROM_STR("XXX")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace content with same size content size 1-> no shift
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1");
    az_span expected = AZ_SPAN_FROM_STR("2");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 0, 1, AZ_SPAN_FROM_STR("2")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace content with same size content size > 1-> no shift
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span expected = AZ_SPAN_FROM_STR("4321");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 0, 4, AZ_SPAN_FROM_STR("4321")) == AZ_OK);

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Append another span after replacing -> builder should keep writing at the end
    uint8_t array[10];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    az_span expected = AZ_SPAN_FROM_STR("1X34AB");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_EXPECT_SUCCESS(_az_span_replace(&builder, 1, 2, AZ_SPAN_FROM_STR("X")));

    TEST_EXPECT_SUCCESS(az_span_append(builder, AZ_SPAN_FROM_STR("AB"), &builder));

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Replace at last position -> insert at the end
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("123");
    az_span expected = AZ_SPAN_FROM_STR("1234");

    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));
    TEST_EXPECT_SUCCESS(_az_span_replace(&builder, 3, 3, AZ_SPAN_FROM_STR("4")));

    az_span const result = builder;
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Fail on buffer override -> try to replace with something bigger than buffer
    uint8_t array[4];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 0, 4, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    // Fail on builder empty -> try to replace content from empty builder
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    TEST_ASSERT(_az_span_replace(&builder, 0, 1, AZ_SPAN_FROM_STR("2")) == AZ_ERROR_ARG);
  }
  {
    // Replace content on empty builder -> insert at the end
    uint8_t array[200];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    TEST_ASSERT(_az_span_replace(&builder, 0, 0, AZ_SPAN_FROM_STR("2")) == AZ_OK);
    az_span const result = builder;
    az_span const expected = AZ_SPAN_FROM_STR("2");
    TEST_ASSERT(az_span_is_equal(result, expected));
  }
  {
    // Fail if trying to replace out of bounds content -> start and end out
    uint8_t array[400];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 30, 31, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    // Fail when trying to replace out of bounds -> end position out
    uint8_t array[40];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 4, 5, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    // Fail when start is greater than end
    uint8_t array[40];
    az_span builder = AZ_SPAN_FROM_BUFFER(array);
    az_span initial_state = AZ_SPAN_FROM_STR("1234");
    TEST_EXPECT_SUCCESS(az_span_append(builder, initial_state, &builder));

    TEST_ASSERT(_az_span_replace(&builder, 3, 1, AZ_SPAN_FROM_STR("4321X")) == AZ_ERROR_ARG);
  }
}
