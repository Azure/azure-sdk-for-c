// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include <az_json.h>

#include <az_test.h>

#include <_az_cfg.h>

void test_json_pointer()
{
  {
    az_span parser = AZ_SPAN_FROM_STR("");
    az_span p;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("Hello");
    az_span p;
    TEST_ASSERT(
        _az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/abc");
    az_span p;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("abc")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i, i);
      TEST_ASSERT(az_span_is_equal(b, AZ_SPAN_FROM_STR("abc")));
    }
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/abc//dffgg21");
    az_span p;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("abc")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i, i);
      TEST_ASSERT(az_span_is_equal(b, AZ_SPAN_FROM_STR("abc")));
    }
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("dffgg21")));
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/ab~1c/dff~0x");
    az_span p;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("ab~1c")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i, i);
      TEST_ASSERT(az_span_is_equal(b, AZ_SPAN_FROM_STR("ab/c")));
    }
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("dff~0x")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        TEST_ASSERT(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i, i);
      TEST_ASSERT(az_span_is_equal(b, AZ_SPAN_FROM_STR("dff~x")));
    }
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/ab~1c/dff~x");
    az_span p;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("ab~1c")));
    TEST_ASSERT(
        _az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/ab~1c/dff~");
    az_span p;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(p, AZ_SPAN_FROM_STR("ab~1c")));
    TEST_ASSERT(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_EOF);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("~");
    uint32_t c;
    TEST_ASSERT(_az_span_reader_read_json_pointer_token_char(&token_parser, &c) == AZ_ERROR_EOF);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("");
    uint32_t c;
    TEST_ASSERT(
        _az_span_reader_read_json_pointer_token_char(&token_parser, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("/");
    uint32_t c;
    TEST_ASSERT(
        _az_span_reader_read_json_pointer_token_char(&token_parser, &c)
        == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("~2");
    uint32_t c;
    TEST_ASSERT(
        _az_span_reader_read_json_pointer_token_char(&token_parser, &c)
        == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
}
