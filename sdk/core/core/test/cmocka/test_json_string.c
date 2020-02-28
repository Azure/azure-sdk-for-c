// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include <az_json.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_json_string(void** state)
{
  (void)state;
  {
    az_span const s = AZ_SPAN_FROM_STR("tr\\\"ue\\t");
    az_span reader = s;
    uint32_t c;
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 't');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 'r');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == '\"');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 'u');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 'e');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == '\t');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\\uFf0F");
    az_span reader = s;
    uint32_t c = { 0 };
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 0xFF0F);
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\\uFf0");
    az_span reader = s;
    uint32_t c;
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_EOF);
  }
}
