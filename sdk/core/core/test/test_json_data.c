// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>
#include <az_str.h>

#include "./az_test.h"

#include <_az_cfg.h>

void test_json_data() {
  // null
  {
    uint8_t buffer[100] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result
        = az_json_to_data(AZ_STR("null"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_NULL);
  }
  // true
  {
    uint8_t buffer[100] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result
        = az_json_to_data(AZ_STR("true"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_BOOLEAN);
    TEST_ASSERT(data->data.boolean == true);
  }
  // false
  {
    uint8_t buffer[100] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result
        = az_json_to_data(AZ_STR("false"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_BOOLEAN);
    TEST_ASSERT(data->data.boolean == false);
  }
  // string
  {
    uint8_t buffer[100] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result = az_json_to_data(
        AZ_STR("\"Hello world!\""), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_STRING);
    TEST_ASSERT(az_span_is_equal(data->data.string, AZ_STR("Hello world!")));
  }
  // number
  {
    uint8_t buffer[100] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result
        = az_json_to_data(AZ_STR("-42.25e+1"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_NUMBER);
    TEST_ASSERT(data->data.number == -422.5);
  }
  // array
  {
    uint8_t buffer[100] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result = az_json_to_data(
        AZ_STR("[0.25,null,false]"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_ARRAY);
    az_json_array const a = data->data.array;
    TEST_ASSERT(a.size == 3);
    {
      az_json_data const item = a.begin[0];
      TEST_ASSERT(item.kind == AZ_JSON_DATA_NUMBER);
      TEST_ASSERT(item.data.number == 0.25);
    }
    {
      az_json_data const item = a.begin[1];
      TEST_ASSERT(item.kind == AZ_JSON_DATA_NULL);
    }
    {
      az_json_data const item = a.begin[2];
      TEST_ASSERT(item.kind == AZ_JSON_DATA_BOOLEAN);
      TEST_ASSERT(item.data.boolean == false);
    }
  }
  // nested array
  {
    uint8_t buffer[1000] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result = az_json_to_data(
        AZ_STR("[[1,[]],2,[[],3]]"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_ARRAY);
    az_json_array const a = data->data.array;
    TEST_ASSERT(a.size == 3);
    {
      az_json_data const item = a.begin[0];
      TEST_ASSERT(item.kind == AZ_JSON_DATA_ARRAY);
      az_json_array const a0 = item.data.array;
      TEST_ASSERT(a0.size == 2);
    }
    {
      az_json_data const item = a.begin[1];
      TEST_ASSERT(item.kind == AZ_JSON_DATA_NUMBER);
      TEST_ASSERT(item.data.number == 2);
    }
    {
      az_json_data const item = a.begin[2];
      TEST_ASSERT(item.kind == AZ_JSON_DATA_ARRAY);
      az_json_array const a2 = item.data.array;
      TEST_ASSERT(a2.size == 2);
    }
  }
}
