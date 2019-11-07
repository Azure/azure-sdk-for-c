// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_pointer_parser.h>
#include <az_span_reader.h>
#include <az_str.h>

#include "./az_test.h"

static void test_json_pointer_parser() {
  {
    az_span_reader parser = az_span_reader_create(AZ_STR(""));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("Hello"));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/abc"));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("abc")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/abc/dffgg21"));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("abc")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("dffgg21")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/ab~1c/dff~0"));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("ab~1c")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("dff~0")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/ab~1c/dff~x"));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("ab~1c")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span_reader parser = az_span_reader_create(AZ_STR("/ab~1c/dff~"));
    az_span p;
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_OK);
    TEST_ASSERT(az_span_eq(p, AZ_STR("ab~1c")));
    TEST_ASSERT(az_json_pointer_parser_get(&parser, &p) == AZ_ERROR_EOF);
  }
}
