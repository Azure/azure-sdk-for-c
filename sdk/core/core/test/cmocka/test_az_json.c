// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>

#include <az_json.h>
#include <az_span.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_json_value(void** state);
void test_json_string(void** state);
void test_json_pointer(void** state);
void test_json_parser(void** state);
void test_json_get_by_pointer(void** state);
void test_json_builder(void** state);

static void test_json_token_null(void** state)
{
  (void)state;
  az_json_token token = az_json_token_null();
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NULL);
  assert_false(token.value.boolean);
  assert_int_equal(token.value.number, 0);

  assert_ptr_equal(az_span_ptr(token.value.string), NULL);
  assert_int_equal(az_span_capacity(token.value.string), 0);
  assert_int_equal(az_span_length(token.value.string), 0);

  assert_ptr_equal(az_span_ptr(token.value.span), NULL);
  assert_int_equal(az_span_capacity(token.value.span), 0);
  assert_int_equal(az_span_length(token.value.span), 0);
}

static void test_json_token_boolean(void** state)
{
  (void)state;
  az_json_token token = az_json_token_boolean(true);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_BOOLEAN);
  assert_true(token.value.boolean);
}

static void test_json_token_number(void** state)
{
  (void)state;
  az_json_token token = az_json_token_number(10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  assert_int_equal(token.value.number, 10);

  token = az_json_token_number(-10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  // Need to create double and not just use -10 directly or it would fail on x86-intel
  double expected = -10;
  assert_int_equal(token.value.number, expected);
}

static void test_json_parser_init(void** state)
{
  (void)state;
  az_span span = AZ_SPAN_FROM_STR("");
  az_json_parser parser;
  assert_int_equal(az_json_parser_init(&parser, span), AZ_OK);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_json_token_null),     cmocka_unit_test(test_json_parser_init),
    cmocka_unit_test(test_json_token_boolean),  cmocka_unit_test(test_json_token_number),
    cmocka_unit_test(test_json_value),          cmocka_unit_test(test_json_string),
    cmocka_unit_test(test_json_pointer),        cmocka_unit_test(test_json_parser),
    cmocka_unit_test(test_json_get_by_pointer), cmocka_unit_test(test_json_builder),
  };

  return cmocka_run_group_tests_name("az_json", tests, NULL, NULL);
}
