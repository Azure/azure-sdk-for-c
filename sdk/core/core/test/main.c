// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_token_state.h>
#include <az_json_read.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int exit_code = 0;

#define TEST_ASSERT(c) \
  do { \
    if(c) { \
      printf("  - `%s`: succeeded\n", #c); \
    } else { \
      fprintf(stderr, "  - `%s`: failed\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while(false);

void json_token_state(az_const_str const input, az_const_str const expected) {
  size_t const size = input.size;
  TEST_ASSERT(size == expected.size);
  az_jts state = AZ_JTS_SPACE;
  for (size_t i = 0; i < size; ++i) {
    state = az_jts_next(state, az_const_str_item(input, i));
    printf(">>%d<<\n", (int)state);
    TEST_ASSERT(state == az_const_str_item(expected, i));
  }
}

int main() {
  json_token_state(AZ_CONST_STR("  "), AZ_CONST_STR("\0\0"));
  json_token_state(AZ_CONST_STR("  12 "), AZ_CONST_STR("\0\0\x32\x32\2"));
  json_token_state(AZ_CONST_STR("  sa "), AZ_CONST_STR("\0\0\1\1\1"));
  json_token_state(AZ_CONST_STR("  12.4 "), AZ_CONST_STR("\0\0\x32\x32\x34\x35\2"));
  json_token_state(AZ_CONST_STR("-0.66e+55 "), AZ_CONST_STR("\x30\x31\x34\x35\x35\x38\x39\x3A\x3A\2"));
  json_token_state(AZ_CONST_STR("-0.66e+ "), AZ_CONST_STR("\x30\x31\x34\x35\x35\x38\x39\1"));
  json_token_state(AZ_CONST_STR(" true "), AZ_CONST_STR("\0\x28\x29\x2A\x2B\2"));
  json_token_state(AZ_CONST_STR(" \"\" "), AZ_CONST_STR("\0\x40\x29\x2A\x2B\2"));

  {
    az_json_state state = az_json_state_create(AZ_CONST_STR("  null  "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NULL);
  }
  return exit_code;
}
