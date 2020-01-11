// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_span_writer_internal.h>

#include "./az_test.h"

#include <_az_cfg.h>

AZ_NODISCARD az_result write_json(void * _, az_span_action const write) {
  (void)_;
  az_json_builder builder = { 0 };

  AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, write));

  // 0 1 0         1         2         3         4         5         6         7         8         9
  // 0
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
  // {"name":true,"foo":["bar",null,0,-12],"int-max":9007199254740991,"esc":"_\"_\\_\b\f\n\r\t_","u":"a\u001Fb"}
  AZ_RETURN_IF_FAILED(az_json_builder_write(&builder, az_json_token_object()));

  AZ_RETURN_IF_FAILED(
      az_json_builder_write_object_member(&builder, AZ_STR("name"), az_json_token_boolean(true)));

  {
    AZ_RETURN_IF_FAILED(
        az_json_builder_write_object_member(&builder, AZ_STR("foo"), az_json_token_array()));
    AZ_RETURN_IF_FAILED(
        az_json_builder_write_array_item(&builder, az_json_token_string(AZ_STR("bar"))));
    AZ_RETURN_IF_FAILED(az_json_builder_write_array_item(&builder, az_json_token_null()));
    AZ_RETURN_IF_FAILED(az_json_builder_write_array_item(&builder, az_json_token_number(0)));
    AZ_RETURN_IF_FAILED(az_json_builder_write_array_item(&builder, az_json_token_number(-12)));
    AZ_RETURN_IF_FAILED(az_json_builder_write_array_close(&builder));
  }

  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder, AZ_STR("int-max"), az_json_token_number(9007199254740991ull)));
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder, AZ_STR("esc"), az_json_token_span(AZ_STR("_\"_\\_\b\f\n\r\t_"))));
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder,
      AZ_STR("u"),
      az_json_token_span(AZ_STR( //
          "a"
          "\x1f"
          "b"))));

  AZ_RETURN_IF_FAILED(az_json_builder_write_object_close(&builder));

  return AZ_OK;
}

AZ_ACTION_FUNC(write_json, void, az_span_writer)

void test_json_builder() {
  {
    size_t size = { 0 };
    TEST_ASSERT(az_span_writer_size(write_json_action(NULL), &size) == AZ_OK);
    TEST_ASSERT(size == 107);
  }
  {
    uint8_t array[200];
    az_span_builder builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(array));
    TEST_ASSERT(write_json(NULL, az_span_builder_append_action(&builder)) == AZ_OK);
    az_span const result = az_span_builder_result(&builder);
    TEST_ASSERT(az_span_is_equal(
        result,
        AZ_STR( //
            "{"
            "\"name\":true,"
            "\"foo\":[\"bar\",null,0,-12],"
            "\"int-max\":9007199254740991,"
            "\"esc\":\"_\\\"_\\\\_\\b\\f\\n\\r\\t_\","
            "\"u\":\"a\\u001Fb\""
            "}")));
  }
}
