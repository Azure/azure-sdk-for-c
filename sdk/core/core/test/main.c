// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_cstr.h>
#include <az_json_number.h>
#include <az_json_string.h>
#include <az_json_keyword.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int exit_code = 0;

#define TEST_ASSERT(c) \
  do { \
    if(c) { printf("  - `%s`: succeeded\n", #c); } else { fprintf(stderr, "- `%s`: failed\n", #c); assert(false); exit_code = 1; } \
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

az_json_string parse_string(az_cstr const str) {
  size_t const len = str.len;
  az_json_string n = az_json_string_create_none();
  for (size_t i = 0; i < len; ++i) {
    n = az_json_string_parse(n, str.p[i]);
  }
  n = az_json_string_parse(n, AZ_STR_TERMINAL);
  return n;
}

void test_string_done(az_cstr const str, int32_t const size, char const next) {
  az_json_string const s = parse_string(str);
  TEST_ASSERT(s.tag == AZ_JSON_STRING_DONE);
  TEST_ASSERT(s.done.position == size);
  TEST_ASSERT(s.done.next == next);
}

az_json_keyword parse_keyword(az_cstr const str) {
  size_t const len = str.len;
  az_json_keyword n = az_json_keyword_create_none();
  for (size_t i = 0; i < len; ++i) {
    n = az_json_keyword_parse(n, str.p[i]);
  }
  n = az_json_keyword_parse(n, AZ_STR_TERMINAL);
  return n;
}

void test_keyword_done(az_cstr const str, az_json_keyword_type const type, char next) {
  az_json_keyword const s = parse_keyword(str);
  TEST_ASSERT(s.tag == AZ_JSON_KEYWORD_DONE);
  TEST_ASSERT(s.done.type == type);
  TEST_ASSERT(s.done.next == next);
}

void test_keyword_error(az_cstr const str) {
  az_json_keyword const s = parse_keyword(str);
  TEST_ASSERT(s.tag == AZ_JSON_KEYWORD_ERROR);
}

int main() {
  printf("\nnumber\n");
  test_number_done(AZ_CSTR("0"), 0, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("-0"), 0, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("123"), 123, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("-123.56"), -123.56, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("-123.56e3"), -123560, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("123.56e-4"), 0.012356, AZ_STR_TERMINAL);
  // test_number_done(AZ_CSTR("-0.056"), -0.056, AZ_STR_TERMINAL);
  test_number_done(AZ_CSTR("1e+10"), 10000000000, AZ_STR_TERMINAL);
  test_number_error(AZ_CSTR("-00"));

  printf("\nstring\n");
  test_string_done(AZ_CSTR("\"Hello world!\""), 12, AZ_STR_TERMINAL);

  printf("\nkeyword\n");
  test_keyword_done(AZ_CSTR("null"), AZ_JSON_KEYWORD_NULL, AZ_STR_TERMINAL);
  test_keyword_done(AZ_CSTR("true"), AZ_JSON_KEYWORD_TRUE, AZ_STR_TERMINAL);
  test_keyword_done(AZ_CSTR("false"), AZ_JSON_KEYWORD_FALSE, AZ_STR_TERMINAL);
  test_keyword_error(AZ_CSTR("fulse"));
  return exit_code;
}
