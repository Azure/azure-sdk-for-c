// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_cstr.h>
#include <az_json_number.h>
#include <az_json_string.h>
#include <az_json_keyword.h>
#include <az_json_token.h>

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
  n = az_json_number_parse(n, ' ');
  return n;
}

void test_number_done(az_cstr const str, double const number) {
  az_json_number const n = parse_number(str);
  printf("%s == %lf", str.p, number);
  TEST_ASSERT(n.tag == AZ_JSON_NUMBER_DONE);
  // printf(" == %.20lf\n == %.20lf\n", n.done.number, number);
  TEST_ASSERT(n.done.number == number);
  TEST_ASSERT(n.done.next == ' ');
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
  n = az_json_string_parse(n, ' ');
  return n;
}

void test_string_done(az_cstr const str, int32_t const size) {
  az_json_string const s = parse_string(str);
  TEST_ASSERT(s.tag == AZ_JSON_STRING_DONE);
  TEST_ASSERT(s.done.position == size);
  TEST_ASSERT(s.done.next == ' ');
}

az_json_keyword parse_keyword(az_cstr const str) {
  size_t const len = str.len;
  az_json_keyword n = az_json_keyword_create_none();
  for (size_t i = 0; i < len; ++i) {
    n = az_json_keyword_parse(n, str.p[i]);
  }
  n = az_json_keyword_parse(n, ' ');
  return n;
}

void test_keyword_done(az_cstr const str, az_json_keyword_type const type) {
  az_json_keyword const s = parse_keyword(str);
  TEST_ASSERT(s.tag == AZ_JSON_KEYWORD_DONE);
  TEST_ASSERT(s.done.type == type);
  TEST_ASSERT(s.done.next == ' ');
}

void test_keyword_error(az_cstr const str) {
  az_json_keyword const s = parse_keyword(str);
  TEST_ASSERT(s.tag == AZ_JSON_KEYWORD_ERROR);
}

typedef struct {
  az_cstr buffer;
  int i;
  az_json_token token;
} input_state;

input_state create_input_state(az_cstr const s) {
  return (input_state){
    .buffer = s,
    .i = 0,
    .token = az_json_token_create_progress(az_json_progress_create_none()),
  };
}

az_json_token get_next_token(input_state *p) {
  for (; p->i < p->buffer.len; ++(p->i)) {
    p->token = az_json_token_parse(p->token, p->buffer.p[p->i]);
    if (p->token.tag != AZ_JSON_TOKEN_PROGRESS) {
      break;
    }
  }
  return p->token;
}

int main() {
  printf("\nnumber\n");
  test_number_done(AZ_CSTR("0"), 0);
  test_number_done(AZ_CSTR("-0"), 0);
  test_number_done(AZ_CSTR("123"), 123);
  test_number_done(AZ_CSTR("-123.56"), -123.56);
  test_number_done(AZ_CSTR("-123.56e3"), -123560);
  test_number_done(AZ_CSTR("123.56e-4"), 0.012356);
  // test_number_done(AZ_CSTR("-0.056"), -0.056);
  test_number_done(AZ_CSTR("1e+10"), 10000000000);
  test_number_error(AZ_CSTR("-00"));

  printf("\nstring\n");
  test_string_done(AZ_CSTR("\"Hello world!\""), 12);

  printf("\nkeyword\n");
  test_keyword_done(AZ_CSTR("null"), AZ_JSON_KEYWORD_NULL);
  test_keyword_done(AZ_CSTR("true"), AZ_JSON_KEYWORD_TRUE);
  test_keyword_done(AZ_CSTR("false"), AZ_JSON_KEYWORD_FALSE);
  test_keyword_error(AZ_CSTR("fulse"));

  printf("\ntoken\n");
  {
    input_state state = create_input_state(AZ_CSTR("     null     -12.41e+1  \"Hello world!\" { "));
    {
      az_json_token token = get_next_token(&state);
      TEST_ASSERT(token.tag == AZ_JSON_TOKEN_KEYWORD);
      TEST_ASSERT(token.keyword.type == AZ_JSON_KEYWORD_NULL);
    }
    {
      az_json_token token = get_next_token(&state);
      TEST_ASSERT(token.tag == AZ_JSON_TOKEN_NUMBER);
      TEST_ASSERT(token.number.number == -124.1);
    }
    {
      az_json_token token = get_next_token(&state);
      TEST_ASSERT(token.tag == AZ_JSON_TOKEN_STRING);
      TEST_ASSERT(token.string.position == 12);
    }
    {
      az_json_token token = get_next_token(&state);
      TEST_ASSERT(token.tag = AZ_JSON_TOKEN_SYMBOL);
      TEST_ASSERT(token.symbol.char_ == '{');
    }
  }

  return exit_code;
}
