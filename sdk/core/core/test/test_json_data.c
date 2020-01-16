// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>
#include <az_str.h>

#include <az_test.h>

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
    TEST_ASSERT(az_json_array_size(a) == 3);
    {
      az_json_data_option const item = az_json_array_get(a, 0);
      TEST_ASSERT(item.has);
      TEST_ASSERT(item.data.kind == AZ_JSON_DATA_NUMBER);
      TEST_ASSERT(item.data.data.number == 0.25);
    }
    {
      az_json_data_option const item = az_json_array_get(a, 1);
      TEST_ASSERT(item.has);
      TEST_ASSERT(item.data.kind == AZ_JSON_DATA_NULL);
    }
    {
      az_json_data_option const item = az_json_array_get(a, 2);
      TEST_ASSERT(item.has);
      TEST_ASSERT(item.data.kind == AZ_JSON_DATA_BOOLEAN);
      TEST_ASSERT(item.data.data.boolean == false);
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
    TEST_ASSERT(az_json_array_size(a) == 3);
    {
      az_json_data_option const item = az_json_array_get(a, 0);
      TEST_ASSERT(item.has);
      TEST_ASSERT(item.data.kind == AZ_JSON_DATA_ARRAY);
      az_json_array const a0 = item.data.data.array;
      TEST_ASSERT(az_json_array_size(a0) == 2);
      {
        az_json_data_option const ii = az_json_array_get(a0, 0);
        TEST_ASSERT(ii.has);
        TEST_ASSERT(ii.data.kind == AZ_JSON_DATA_NUMBER);
        TEST_ASSERT(ii.data.data.number == 1);
      }
      {
        az_json_data_option const ii = az_json_array_get(a0, 1);
        TEST_ASSERT(ii.has);
        TEST_ASSERT(ii.data.kind == AZ_JSON_DATA_ARRAY);
        TEST_ASSERT(az_json_array_size(ii.data.data.array) == 0);
      }
    }
    {
      az_json_data_option const item = az_json_array_get(a, 1);
      TEST_ASSERT(item.has);
      TEST_ASSERT(item.data.kind == AZ_JSON_DATA_NUMBER);
      TEST_ASSERT(item.data.data.number == 2);
    }
    {
      az_json_data_option const item = az_json_array_get(a, 2);
      TEST_ASSERT(item.has);
      TEST_ASSERT(item.data.kind == AZ_JSON_DATA_ARRAY);
      az_json_array const a2 = item.data.data.array;
      TEST_ASSERT(az_json_array_size(a2) == 2);
    }
  }
  // object
  {
    uint8_t buffer[200] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result = az_json_to_data(
        AZ_STR("{\"foo\":45,\"bar\":true}"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_OBJECT);
    az_json_object const o = data->data.object;
    TEST_ASSERT(az_json_object_size(o) == 2);
    {
      az_json_object_member_option const m = az_json_object_get(o, 0);
      TEST_ASSERT(m.has);
      TEST_ASSERT(az_span_is_equal(m.data.name, AZ_STR("foo")));
      TEST_ASSERT(m.data.value.kind == AZ_JSON_DATA_NUMBER);
      TEST_ASSERT(m.data.value.data.number == 45);
    }
    {
      az_json_object_member_option const m = az_json_object_get(o, 1);
      TEST_ASSERT(m.has);
      TEST_ASSERT(az_span_is_equal(m.data.name, AZ_STR("bar")));
      TEST_ASSERT(m.data.value.kind == AZ_JSON_DATA_BOOLEAN);
      TEST_ASSERT(m.data.value.data.boolean == true);
    }
  }
  // nested object
  {
    uint8_t buffer[200] = { 0 };
    az_json_data const * data = { 0 };
    az_result const result = az_json_to_data(
        AZ_STR("{\"foo\":{\"bar2\":\"hello\"},\"bar\":[\"world\"]}"),
        (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer),
        &data);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(data->kind == AZ_JSON_DATA_OBJECT);
    az_json_object const o = data->data.object;
    TEST_ASSERT(az_json_object_size(o) == 2);
    {
      az_json_object_member_option const m = az_json_object_get(o, 0);
      TEST_ASSERT(m.has);
      TEST_ASSERT(az_span_is_equal(m.data.name, AZ_STR("foo")));
      TEST_ASSERT(m.data.value.kind == AZ_JSON_DATA_OBJECT);
      az_json_object mo = m.data.value.data.object;
      TEST_ASSERT(az_json_object_size(mo) == 1);
      {
        az_json_object_member_option const mom = az_json_object_get(mo, 0);
        TEST_ASSERT(mom.has);
        TEST_ASSERT(az_span_is_equal(mom.data.name, AZ_STR("bar2")));
        TEST_ASSERT(mom.data.value.kind == AZ_JSON_DATA_STRING);
        TEST_ASSERT(az_span_is_equal(mom.data.value.data.string, AZ_STR("hello")));
      }
    }
    {
      az_json_object_member_option const m = az_json_object_get(o, 1);
      TEST_ASSERT(m.has);
      TEST_ASSERT(az_span_is_equal(m.data.name, AZ_STR("bar")));
      TEST_ASSERT(m.data.value.kind == AZ_JSON_DATA_ARRAY);
      az_json_array const a = m.data.value.data.array;
      TEST_ASSERT(az_json_array_size(a) == 1);
      {
        az_json_data_option const i = az_json_array_get(a, 0);
        TEST_ASSERT(i.has);
        TEST_ASSERT(i.data.kind == AZ_JSON_DATA_STRING);
        TEST_ASSERT(az_span_is_equal(i.data.data.string, AZ_STR("world")));
      }
    }
  }
}
