// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include <az_json.h>
#include <az_test.h>

#include <_az_cfg.h>

void test_json_string()
{
  {
    az_span const s = AZ_SPAN_FROM_STR("tr\\\"ue\\t");
    az_span reader = s;
    uint32_t c;
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == 't');
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == 'r');
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == '\"');
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == 'u');
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == 'e');
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == '\t');
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\\uFf0F");
    az_span reader = s;
    uint32_t c = { 0 };
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    TEST_ASSERT(c == 0xFF0F);
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\\uFf0");
    az_span reader = s;
    uint32_t c;
    TEST_ASSERT(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_EOF);
  }
}
