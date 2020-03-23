// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_private.h"
#include "az_test_definitions.h"
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
    assert_true(az_json_token_get_boolean(&json_boolean, &boolean_value) == AZ_OK);
    assert_true(boolean_value);
  }
  // boolean from number
  {
    bool boolean_value = false;
    assert_true(az_json_token_get_boolean(&json_number, &boolean_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // string from string
  {
    az_span string_value = { 0 };
    assert_true(az_json_token_get_string(&json_string, &string_value) == AZ_OK);
    assert_true(az_span_is_content_equal(string_value, AZ_SPAN_FROM_STR("Hello")));
  }
  // string from boolean
  {
    az_span string_value = { 0 };
    assert_true(az_json_token_get_string(&json_boolean, &string_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // number from number
  {
    double number_value = 0.79;
    uint64_t const* const number_value_bin_rep_view = (uint64_t*)&number_value;

    assert_true(az_json_token_get_number(&json_number, &number_value) == AZ_OK);

    double const expected_value = -42.3;
    uint64_t const* const expected_value_bin_rep_view = (uint64_t const*)&expected_value;

    assert_true(*number_value_bin_rep_view == *expected_value_bin_rep_view);
  }
  // number from string
  {
    double number_value = 0.79;
    assert_true(az_json_token_get_number(&json_string, &number_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }
}
