// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_value.h>
#include <az_str.h>

#include "./az_test.h"

static void test_json_value() {
  az_json_value const json_boolean = az_json_value_create_boolean(true);
  az_json_value const json_number = az_json_value_create_number(-42.3);
  az_json_value const json_string = az_json_value_create_string(AZ_STR("Hello"));

  // boolean from boolean
  {
    bool boolean_value = false;
    TEST_ASSERT(az_json_value_get_boolean(&json_boolean, &boolean_value) == AZ_OK);
    TEST_ASSERT(boolean_value == true);
  }
  // boolean from number
  {
    bool boolean_value = false;
    TEST_ASSERT(az_json_value_get_boolean(&json_number, &boolean_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // string from string
  {
    az_span string_value = { 0 };
    TEST_ASSERT(az_json_value_get_string(&json_string, &string_value) == AZ_OK);
    TEST_ASSERT(az_span_eq(string_value, AZ_STR("Hello")));
  }
  // string from boolean
  {
    az_span string_value = { 0 };
    TEST_ASSERT(az_json_value_get_string(&json_boolean, &string_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // number from number
  {
    double number_value = 0.79;
    TEST_ASSERT(az_json_value_get_number(&json_number, &number_value) == AZ_OK);
    TEST_ASSERT(number_value == -42.3);
  }
  // number from string
  {
    double number_value = 0.79;
    TEST_ASSERT(az_json_value_get_number(&json_string, &number_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }
}
