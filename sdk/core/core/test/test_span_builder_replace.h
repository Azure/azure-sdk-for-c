// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_span_builder.h>
#include <az_str.h>

#include "./az_test.h"

static void test_span_builder_replace() {
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("12345678");
    az_span expected = AZ_STR("1X78");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 1, 6, AZ_STR("X")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("12345678");
    az_span expected = AZ_STR("12X345678");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 2, 2, AZ_STR("X")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("12345678");
    az_span expected = AZ_STR("1234567890");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 8, 8, AZ_STR("90")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("12345678");
    az_span expected = AZ_STR("X");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 8, AZ_STR("X")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("12345678");
    az_span expected = AZ_STR("X12345678X");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 8, AZ_STR("X12345678X")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("12345678");
    az_span expected = AZ_STR("XXX12345678");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 0, AZ_STR("XXX")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("1");
    az_span expected = AZ_STR("2");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 1, AZ_STR("2")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[4];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("1234");
    az_span expected = AZ_STR("4321");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 4, AZ_STR("4321")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[4];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("123");
    az_span expected = AZ_STR("1234");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 3, 3, AZ_STR("4")) == AZ_OK);

    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_eq(result, expected));
  }
  {
    uint8_t array[4];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    az_span initial_state = AZ_STR("1234");
    az_span expected = AZ_STR("4321");
    az_result ignore = az_span_builder_append(&builder, initial_state);

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 4, AZ_STR("4321X")) == AZ_ERROR_ARG);
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));

    TEST_ASSERT(az_span_builder_replace(&builder, 0, 1, AZ_STR("2")) == AZ_ERROR_ARG);
  }
}
