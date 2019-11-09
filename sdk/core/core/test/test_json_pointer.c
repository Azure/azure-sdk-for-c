// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_str.h>
#include <az_span_reader.h>

#include "./az_test.h"

#include <_az_cfg.h>

void test_json_pointer() {
  {
    az_span_reader parser = az_span_reader_create(AZ_STR(""));
    az_span p;
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("Hello"));
    az_span p;
    TEST_ASSERT(
        az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/abc"));
    az_span p;
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("abc")));
    // test az_json_pointer_token_parser_get
    {
      az_span_reader token_parser = az_span_reader_create(p);
      uint8_t buffer[10];
      int i = 0;
      while (true) {
        az_result const result
            = az_span_reader_read_json_pointer_token_char(&token_parser, buffer + i);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        ++i;
      }
      az_span const b = { .begin = buffer, .size = i };
      TEST_ASSERT(az_span_eq(b, AZ_STR("abc")));
    }
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/abc//dffgg21"));
    az_span p;
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("abc")));
    // test az_json_pointer_token_parser_get
    {
      az_span_reader token_parser = az_span_reader_create(p);
      uint8_t buffer[10];
      int i = 0;
      while (true) {
        az_result const result
            = az_span_reader_read_json_pointer_token_char(&token_parser, buffer + i);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        ++i;
      }
      az_span const b = { .begin = buffer, .size = i };
      TEST_ASSERT(az_span_eq(b, AZ_STR("abc")));
    }
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("")));
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("dffgg21")));
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/ab~1c/dff~0x"));
    az_span p;
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("ab~1c")));
    // test az_json_pointer_token_parser_get
    {
      az_span_reader token_parser = az_span_reader_create(p);
      uint8_t buffer[10];
      int i = 0;
      while (true) {
        az_result const result
            = az_span_reader_read_json_pointer_token_char(&token_parser, buffer + i);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        ++i;
      }
      az_span const b = { .begin = buffer, .size = i };
      TEST_ASSERT(az_span_eq(b, AZ_STR("ab/c")));
    }
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("dff~0x")));
    // test az_json_pointer_token_parser_get
    {
      az_span_reader token_parser = az_span_reader_create(p);
      uint8_t buffer[10];
      int i = 0;
      while (true) {
        az_result const result
            = az_span_reader_read_json_pointer_token_char(&token_parser, buffer + i);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        ++i;
      }
      az_span const b = { .begin = buffer, .size = i };
      TEST_ASSERT(az_span_eq(b, AZ_STR("dff~x")));
    }
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/ab~1c/dff~x"));
    az_span p;
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("ab~1c")));
    TEST_ASSERT(
        az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/ab~1c/dff~"));
    az_span p;
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("ab~1c")));
    TEST_ASSERT(az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_EOF);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span_reader token_parser = az_span_reader_create(AZ_STR("~"));
    char c;
    TEST_ASSERT(az_span_reader_read_json_pointer_token_char(&token_parser, &c) == AZ_ERROR_EOF);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span_reader token_parser = az_span_reader_create(AZ_STR(""));
    char c;
    TEST_ASSERT(
        az_span_reader_read_json_pointer_token_char(&token_parser, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span_reader token_parser = az_span_reader_create(AZ_STR("/"));
    char c;
    TEST_ASSERT(
        az_span_reader_read_json_pointer_token_char(&token_parser, &c)
        == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span_reader token_parser = az_span_reader_create(AZ_STR("~2"));
    char c;
    TEST_ASSERT(
        az_span_reader_read_json_pointer_token_char(&token_parser, &c)
        == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
}
