// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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
    // {"name":true,"foo":["bar",null,0,-12],"int-max":9007199254740991,"esc":"_\"_\\_\b\f\n\r\t_","u":"a\u001Fb"}
    TEST_EXPECT_SUCCESS(az_json_builder_append_token(&builder, az_json_token_object()));

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("name"), az_json_token_boolean(true)));

    {
      TEST_EXPECT_SUCCESS(
          az_json_builder_append_object(&builder, AZ_SPAN_FROM_STR("foo"), az_json_token_array()));
      az_result e = az_json_builder_append_array_item(
          &builder, az_json_token_string(AZ_SPAN_FROM_STR("bar")));
      TEST_EXPECT_SUCCESS(e);
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_null()));
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(0)));
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_item(&builder, az_json_token_number(-12)));
      TEST_EXPECT_SUCCESS(az_json_builder_append_array_close(&builder));
    }

    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder, AZ_SPAN_FROM_STR("int-max"), az_json_token_number(9007199254740991ull)));
    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder,
        AZ_SPAN_FROM_STR("esc"),
        az_json_token_span(AZ_SPAN_FROM_STR("_\"_\\_\b\f\n\r\t_"))));
    TEST_EXPECT_SUCCESS(az_json_builder_append_object(
        &builder,
        AZ_SPAN_FROM_STR("u"),
        az_json_token_span(AZ_SPAN_FROM_STR( //
            "a"
            "\x1f"
            "b"))));

    TEST_EXPECT_SUCCESS(az_json_builder_append_object_close(&builder));

    assert_true(az_span_is_equal(
        builder._internal.json,
        AZ_SPAN_FROM_STR( //
            "{"
            "\"name\":true,"
            "\"foo\":[\"bar\",null,0,-12],"
            "\"int-max\":9007199254740991,"
            "\"esc\":\"_\\\"_\\\\_\\b\\f\\n\\r\\t_\","
            "\"u\":\"a\\u001Fb\""
            "}")));
  }
}
