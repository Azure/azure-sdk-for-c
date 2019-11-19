// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>
#include <az_span.h>
#include <az_span_writer.h>s

#include "./az_test.h"

#include <_az_cfg.h>

AZ_NODISCARD az_result write_json(void * _, az_span_action const write) {
  (void)_;
  az_json_builder builder = { 0 };

  AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, write));

  // 0         1         2
  // 0123456789012345678901234
  // {"name":true,"foo":"bar"}
  AZ_RETURN_IF_FAILED(az_json_builder_write(&builder, az_json_value_create_object()));
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder, AZ_STR("name"), az_json_value_create_boolean(true)));
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder, AZ_STR("foo"), az_json_value_create_string(AZ_STR("bar"))));
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_close(&builder));
  return AZ_OK;
}

AZ_ACTION_FUNC(write_json, void, az_span_writer)

void test_json_builder() {
  size_t size = { 0 };
  TEST_ASSERT(az_span_writer_size(write_json_action(NULL), &size) == AZ_OK);
  TEST_ASSERT(size == 25);
}
