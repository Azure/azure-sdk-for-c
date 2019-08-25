// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_cstr.h>
#include <az_json_number.h>

#include <stdio.h>
#include <stdbool.h>

int exit_code = 0;

#define TEST_ASSERT(c) \
  do { \
    if(c) { printf("  - `%s`: succeeded\n", #c); } else { fprintf(stderr, "- `%s`: failed\n", #c); exit_code = 1; } \
  } while(false);

az_json_number parse_number(az_cstr const str) {
  size_t const len = str.len;
  az_json_number n = az_json_number_create_none();
  for (size_t i = 0; i < len; ++i) {
    n = az_json_number_parse(n, str.p[i]);
  }
  n = az_json_number_parse(n, AZ_STR_TERMINAL);
  return n;
}

void test_number_done(az_cstr const str, double const number, char next) {
  az_json_number const n = parse_number(str);
  printf("%s == %lf", str.p, number);
  TEST_ASSERT(n.tag == AZ_JSON_NUMBER_DONE);
  // printf(" == %.20lf\n == %.20lf\n", n.done.number, number);
  TEST_ASSERT(n.done.number == number);
  TEST_ASSERT(n.done.next == next);
}

void test_number_error(az_cstr const str) {
  az_json_number const n = parse_number(str);
  printf("error: %s\n", str.p);
  TEST_ASSERT(n.tag == AZ_JSON_NUMBER_ERROR);
}

int main() {
  test_number_done(AZ_CSTR("0"), 0, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("-0"), 0, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("123"), 123, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("-123.56"), -123.56, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("-123.56e3"), -123560, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("123.56e-4"), 0.012356, AZ_STR_TERMINAL);
  // test_number_done(AZ_CSTR("-0.056"), -0.056, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("1e+10"), 10000000000, AZ_STR_TERMINAL);
  test_number_error(AZ_CSTR("-00"));
  return exit_code;
}

/*
az_error json_parse(az_cstr s) {
  size_t i = 0;
  az_json_value value;
  return az_json_parse_value(s, &i, &value);
}

AZ_CSTR(json, "");
AZ_CSTR(json_ws, "   \r \n \t ");
AZ_CSTR(json_null, "null");
AZ_CSTR(json_nulx, "nulx");
AZ_CSTR(json_false, "false");
AZ_CSTR(json_true, "true");
AZ_CSTR(json_null_space, "null  ");
AZ_CSTR(json_string, "\"Hello world!\"");
AZ_CSTR(json_invalid_string, "\"Hello world\n\"");
AZ_CSTR(json_no_end_string, "\"Hello");

int main() {
  ASSERT(json_parse(json) == AZ_JSON_ERROR_UNEXPECTED_END);
  ASSERT(json_parse(json_ws) == AZ_JSON_ERROR_UNEXPECTED_END);
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_null, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_NULL);
  }
  ASSERT(json_parse(json_nulx) == AZ_JSON_ERROR_UNEXPECTED_SYMBOL);
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_false, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_FALSE);
  }
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_true, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_TRUE);
  }
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_null_space, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_NULL);
  }
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_string, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_STRING);
    ASSERT(value.string.begin == 1);
    // "Hello world!"
    // 01234567890123
    ASSERT(value.string.end == 13);
  }
  ASSERT(json_parse(json_invalid_string) == AZ_JSON_ERROR_UNEXPECTED_SYMBOL);
  ASSERT(json_parse(json_no_end_string) == AZ_JSON_ERROR_UNEXPECTED_END);
  return result;
}
*/
