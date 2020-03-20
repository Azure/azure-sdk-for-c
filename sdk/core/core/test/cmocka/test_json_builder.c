// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include "az_test_definitions.h"
#include <az_json.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

void test_json_builder(void** state)
{
  (void)state;
  {
    uint8_t array[200];
    az_json_builder builder = { 0 };

    TEST_EXPECT_SUCCESS(az_json_builder_init(&builder, AZ_SPAN_FROM_BUFFER(array)));

    // 0___________________________________________________________________________________________________1
    // 0_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
    // {"name":true,"foo":["bar",null,0,-12],"int-max":9007199254740991}
    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_start()));

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("name"), az_json_token_boolean(true)));

    {
      TEST_EXPECT_SUCCESS(az_json_builder_append_object(
          &builder, AZ_SPAN_FROM_STR("foo"), az_json_token_array_start()));
      az_result e = az_json_builder_append_array_item(
          &builder, az_json_token_string(AZ_SPAN_FROM_STR("bar")));
      TEST_EXPECT_SUCCESS(e);
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_null()));
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(0)));
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(-12)));
      TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_array_end()));
    }

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("int-max"), az_json_token_number(9007199254740991ull)));

    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_end()));

    assert_true(az_span_is_content_equal(
        builder._internal.json,
        AZ_SPAN_FROM_STR( //
            "{"
            "\"name\":true,"
            "\"foo\":[\"bar\",null,0,-12],"
            "\"int-max\":9007199254740991"
            "}")));
  }
  {
    // json with AZ_JSON_TOKEN_STRING
    uint8_t array[200];
    az_json_builder builder = { 0 };
    TEST_EXPECT_SUCCESS(az_json_builder_init(&builder, AZ_SPAN_FROM_BUFFER(array)));

    // this json { "span": "\" } would be written as { "span": \"" } with no extra scaping
    uint8_t single_char[1] = { 92 }; // char = '\'
    az_span single_span = AZ_SPAN_FROM_INITIALIZED_BUFFER(single_char);

    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_start()));

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("span"), az_json_token_string(single_span)));

    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_end()));

    assert_true(az_span_is_content_equal(
        builder._internal.json,
        AZ_SPAN_FROM_STR( //
            "{"
            "\"span\":\"\\\""
            "}")));
  }
  {
    // json with array and object inside
    uint8_t array[200];
    az_json_builder builder = { 0 };
    TEST_EXPECT_SUCCESS(az_json_builder_init(&builder, AZ_SPAN_FROM_BUFFER(array)));

    // this json { "array": [1, 2, "sd": {}, 3 ] }
    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_start()));

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("array"), az_json_token_array_start()));

    TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(1)));
    TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(2)));

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("sd"), az_json_token_object_start()));
    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_end()));

    TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(3)));

    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_array_end()));
    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object_end()));

    assert_true(az_span_is_content_equal(
        builder._internal.json,
        AZ_SPAN_FROM_STR( //
            "{"
            "\"array\":[1,2,\"sd\":{},3]"
            "}")));
  }
}
