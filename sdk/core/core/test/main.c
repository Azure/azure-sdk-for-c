// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_cstr.h>
#include <az_json_read.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int exit_code = 0;

#define TEST_ASSERT(c) \
  do { \
    if(c) { printf("  - `%s`: succeeded\n", #c); } else { fprintf(stderr, "- `%s`: failed\n", #c); assert(false); exit_code = 1; } \
  } while(false);

int main() {

  printf("\nparse value\n");
  {
    size_t i = 0;
    az_json_value value;
    az_error result = az_json_read_value(AZ_CSTR("null"), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NULL);
  }

  {
    size_t i = 0;
    az_json_value value;
    az_error result = az_json_read_value(AZ_CSTR("    false "), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_BOOLEAN);
    TEST_ASSERT(value.boolean == false);
  }

  {
    size_t i = 0;
    az_json_value value;
    az_error result = az_json_read_value(AZ_CSTR("    {   } "), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_OBJECT);
    TEST_ASSERT(value.object == false);
  }

  {
    size_t i = 0;
    az_json_value value;
    az_error result = az_json_read_value(AZ_CSTR("    {  \"a\" : 4 } "), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_OBJECT);
    TEST_ASSERT(value.object == true);
  }

  return exit_code;
}
