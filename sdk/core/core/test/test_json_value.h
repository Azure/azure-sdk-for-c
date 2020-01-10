// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_token.h>
#include <az_str.h>

#include "./az_test.h"

#include <_az_cfg.h>

static void test_json_value() {
  az_json_token const json_boolean = az_json_token_boolean(true);
  az_json_token const json_number = az_json_token_number(-42.3);
  az_json_token const json_string = az_json_token_string(AZ_STR("Hello"));

  // boolean from boolean
  {
    bool boolean_value = false;
    TEST_ASSERT(az_json_token_get_boolean(json_boolean, &boolean_value) == AZ_OK);
    TEST_ASSERT(boolean_value == true);
  }
  // boolean from number
  {
    bool boolean_value = false;
    TEST_ASSERT(az_json_token_get_boolean(json_number, &boolean_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // string from string
  {
    az_span string_value = { 0 };
    TEST_ASSERT(az_json_token_get_string(json_string, &string_value) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(string_value, AZ_STR("Hello")));
  }
  // string from boolean
  {
    az_span string_value = { 0 };
    TEST_ASSERT(az_json_token_get_string(json_boolean, &string_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // number from number
  {
    double number_value = 0.79;
    TEST_ASSERT(az_json_token_get_number(json_number, &number_value) == AZ_OK);
    TEST_ASSERT(number_value == -42.3);
  }
  // number from string
  {
    double number_value = 0.79;
    TEST_ASSERT(az_json_token_get_number(json_string, &number_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }
}
