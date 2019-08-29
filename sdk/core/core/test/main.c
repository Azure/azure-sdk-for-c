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
    az_error const result = az_json_read_value(AZ_CSTR("null"), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NULL);
  }

  {
    size_t i = 0;
    az_json_value value;
    az_error const result = az_json_read_value(AZ_CSTR("    false "), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_BOOLEAN);
    TEST_ASSERT(value.boolean == false);
  }

  {
    size_t i = 0;
    az_json_value value;
    az_error const result = az_json_read_value(AZ_CSTR("    {   } "), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_OBJECT);
    TEST_ASSERT(value.object == true);
  }

  {
    //                        01234567 89 0123456
    az_cstr buffer = AZ_CSTR("    {  \"a\" : 4 } ");
    size_t i = 0;
    az_json_value value;
    {
      az_error const result = az_json_read_value(buffer, &i, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.tag == AZ_JSON_OBJECT);
      TEST_ASSERT(value.object == false);
    }
    {
      az_json_property property;
      az_error const result = az_json_read_object_property(buffer, &i, &property);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(property.name.begin = 8);
      TEST_ASSERT(property.name.end = 9);
      TEST_ASSERT(property.value.tag == AZ_JSON_NUMBER);
    }
    {
      bool end;
      az_error result = az_json_read_object_end(buffer, &i, &end);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(end);
    }
  }

  {
    size_t i = 0;
    az_json_value value;
    //                                                  01234 567890 12
    az_error const result = az_json_read_value(AZ_CSTR("    \"hello\"  "), &i, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_STRING);
    TEST_ASSERT(value.string.begin == 5);
    TEST_ASSERT(value.string.end == 10);
  }

  return exit_code;
}
