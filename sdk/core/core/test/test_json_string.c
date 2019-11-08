// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "./test_json_string.h"

#include "./az_test.h"

#include <az_json_string.h>
#include <az_str.h>

#include <_az_cfg.h>

void test_json_string() {
  {
    az_span const s = AZ_STR("tr\\\"ue\\t");
    az_span_reader reader = az_span_reader_create(s);
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == 't');
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == 'r');
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == '\"');
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == 'u');
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == 'e');
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == '\t');
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_STR("\\uFf0F");
    az_span_reader reader = az_span_reader_create(s);
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == 0xFF0F);
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_STR("\\uFf0");
    az_span_reader reader = az_span_reader_create(s);
    TEST_ASSERT(az_span_reader_get_json_string_char(&reader) == AZ_ERROR_EOF);
  }
}
