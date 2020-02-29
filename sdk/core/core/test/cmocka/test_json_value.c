// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_json_value(void** state)
{
  (void)state;

  az_json_token const json_boolean = az_json_token_boolean(true);
  az_json_token const json_number = az_json_token_number(-42.3);
  az_json_token const json_string = az_json_token_string(AZ_SPAN_FROM_STR("Hello"));

  // boolean from boolean
  {
    bool boolean_value = false;
    assert_true(az_json_token_get_boolean(json_boolean, &boolean_value) == AZ_OK);
    assert_true(boolean_value);
  }
  // boolean from number
  {
    bool boolean_value = false;
    assert_true(az_json_token_get_boolean(json_number, &boolean_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // string from string
  {
    az_span string_value = { 0 };
    assert_true(az_json_token_get_string(json_string, &string_value) == AZ_OK);
    assert_true(az_span_is_equal(string_value, AZ_SPAN_FROM_STR("Hello")));
  }
  // string from boolean
  {
    az_span string_value = { 0 };
    assert_true(az_json_token_get_string(json_boolean, &string_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // number from number
  {
    double number_value = 0.79;
    assert_true(az_json_token_get_number(json_number, &number_value) == AZ_OK);
    assert_true(number_value == -42.3);
  }
  // number from string
  {
    double number_value = 0.79;
    assert_true(az_json_token_get_number(json_string, &number_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }
}
