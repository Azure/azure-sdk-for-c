// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_pnp_client.h"
#include <az_test_log.h>
#include <az_test_precondition.h>
#include <az_test_span.h>
#include <azure/core/az_log.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_pnp_client.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 128

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR("my_device");
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR("myiothub.azure-devices.net");
static const az_span test_model_id
    = AZ_SPAN_LITERAL_FROM_STR("dtmi:YOUR_COMPANY_NAME_HERE:sample_device;1");
static az_span test_component_one = AZ_SPAN_LITERAL_FROM_STR("component_one");
static az_span test_component_two = AZ_SPAN_LITERAL_FROM_STR("component_two");
static az_span test_components[]
    = { AZ_SPAN_LITERAL_FROM_STR("component_one"), AZ_SPAN_LITERAL_FROM_STR("component_two") };
static const int32_t test_components_length = sizeof(test_components) / sizeof(test_components[0]);
static const az_span test_device_request_id = AZ_SPAN_LITERAL_FROM_STR("id_one");
static const az_span test_property_received_topic_desired_success
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/PATCH/properties/desired/?$version=id_one");
static const az_span test_property_received_topic_fail
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/rez/200");
static const az_span test_property_received_topic_prefix_fail
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/contoso/res/200");

static const az_span test_property_received_get_response
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/res/200/?$rid=id_one");
static const az_span test_property_reported_props_success_response
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/res/204/?$rid=id_one&$version=16");

static const char test_correct_property_get_request_topic[] = "$iothub/twin/GET/?$rid=id_one";
static const char test_correct_property_patch_pub_topic[]
    = "$iothub/twin/PATCH/properties/reported/?$rid=id_one";

/*

{
  "component_one": {
    "prop_one": 1,
    "prop_two": "string"
  },
  "component_two": {
    "prop_three": 45,
    "prop_four": "string"
  },
  "not_component": 42,
  "$version": 5
}

*/
static const az_span test_property_payload = AZ_SPAN_LITERAL_FROM_STR(
    "{\"component_one\":{\"prop_one\":1,\"prop_two\":\"string\"},\"component_two\":{\"prop_three\":"
    "45,\"prop_four\":\"string\"},\"not_component\":42,\"$version\":5}");

/*

{
  "component_one": {
    "prop_one": 1,
    "prop_two": {
      "prop_one": "value_one",
      "prop_two": "value_two"
    }
  },
  "component_two": {
    "prop_three": 45,
    "prop_four": "string"
  },
  "not_component": {
    "prop_one": "value_one",
    "prop_two": "value_two"
  },
  "$version": 5
}

*/
static const az_span test_property_payload_with_user_object = AZ_SPAN_LITERAL_FROM_STR(
    "{\"component_one\":{\"prop_one\":1,\"prop_two\":{\"prop_one\":\"value_one\",\"prop_two\":"
    "\"value_two\"}},\"component_two\":{\"prop_three\":45,\"prop_four\":\"string\"},\"not_"
    "component\":{\"prop_one\":\"value_one\",\"prop_two\":\"value_two\"},\"$version\":5}");

/*

{
    "desired": {
        "thermostat1": {
            "__t": "c",
            "targetTemperature": 47
        },
        "$version": 4
    },
    "reported": {
        "manufacturer": "Sample-Manufacturer",
        "model": "pnp-sample-Model-123",
        "swVersion": "1.0.0.0",
        "osName": "Contoso"
    }
}

*/
static const az_span test_property_payload_two = AZ_SPAN_LITERAL_FROM_STR(
    "{\"desired\":{\"thermostat1\":{\"__t\":\"c\",\"targetTemperature\":47},\"$version\":4},"
    "\"reported\":{\"manufacturer\":\"Sample-Manufacturer\",\"model\":\"pnp-sample-Model-123\","
    "\"swVersion\":\"1.0.0.0\",\"osName\":\"Contoso\"}}");

/*

{
  "reported": {
    "manufacturer": "Sample-Manufacturer",
    "model": "pnp-sample-Model-123",
    "swVersion": "1.0.0.0",
    "osName": "Contoso"
  },
  "desired": {
    "$version": 4,
    "thermostat1": {
      "targetTemperature": 47,
      "__t": "c"
    }
  }
}

*/
static const az_span test_property_payload_out_of_order = AZ_SPAN_LITERAL_FROM_STR(
    "{\"reported\":{\"manufacturer\":\"Sample-Manufacturer\",\"model\":\"pnp-sample-Model-123\","
    "\"swVersion\":\"1.0.0.0\",\"osName\":\"Contoso\"},\"desired\":{\"$version\":4,\"thermostat1\":"
    "{\"targetTemperature\":47,\"__t\":\"c\"}}}");

/*

{
  "desired": {
    "thermostat2": {
      "__t": "c",
      "targetTemperature": 50
    },
    "thermostat1": {
      "__t": "c",
      "targetTemperature": 90
      "manufacturer": "Sample-Manufacturer",
      "model": "pnp-sample-Model-123",
      "swVersion": "1.0.0.0",
      "osName": "Contoso",
    },
    "targetTemperature": 54,
    "$version": 30
  },
  "reported": {
    "manufacturer": "Sample-Manufacturer",
    "model": "pnp-sample-Model-123",
    "swVersion": "1.0.0.0",
    "osName": "Contoso",
    "processorArchitecture": "Contoso-Arch-64bit",
    "processorManufacturer": "Processor Manufacturer(TM)",
    "totalStorage": 1024,
    "totalMemory": 128
  }
}

*/
static const az_span test_property_payload_long = AZ_SPAN_LITERAL_FROM_STR(
    "{\"desired\":{\"thermostat2\":{\"__t\":\"c\",\"targetTemperature\":50},\"thermostat1\":{\"__"
    "t\":\"c\",\"targetTemperature\":90},\"targetTemperature\":54,\"$version\":30},\"reported\":{"
    "\"manufacturer\":\"Sample-Manufacturer\",\"model\":\"pnp-sample-Model-123\",\"swVersion\":\"1."
    "0.0.0\",\"osName\":\"Contoso\",\"processorArchitecture\":\"Contoso-Arch-64bit\","
    "\"processorManufacturer\":\"Processor "
    "Manufacturer(TM)\",\"totalStorage\":1024,\"totalMemory\":128}}");
static az_span test_temp_component_one = AZ_SPAN_LITERAL_FROM_STR("thermostat1");
static az_span test_temp_component_two = AZ_SPAN_LITERAL_FROM_STR("thermostat2");
static az_span test_temperature_components[]
    = { AZ_SPAN_LITERAL_FROM_STR("thermostat1"), AZ_SPAN_LITERAL_FROM_STR("thermostat2") };
static const int32_t test_temperature_components_length
    = sizeof(test_temperature_components) / sizeof(test_temperature_components[0]);

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_iot_pnp_client_property_document_get_publish_topic_NULL_client_fails()
{
  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_document_get_publish_topic(
      NULL, test_device_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_pnp_client_property_document_get_publish_topic_NULL_request_id_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_create(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_document_get_publish_topic(
      &client, test_bad_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_pnp_client_property_document_get_publish_topic_NULL_span_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_document_get_publish_topic(
      &client, test_device_request_id, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_pnp_client_property_document_get_publish_topic_NULL_out_span_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_document_get_publish_topic(
      &client, test_device_request_id, test_buf, 0, &test_length));
}

static void test_az_iot_pnp_client_property_patch_get_publish_topic_NULL_client_fails()
{
  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_patch_get_publish_topic(
      NULL, test_device_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_pnp_client_property_patch_get_publish_topic_invalid_request_id_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_create(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_patch_get_publish_topic(
      &client, test_bad_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_pnp_client_property_patch_get_publish_topic_NULL_char_buf_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_patch_get_publish_topic(
      &client, test_device_request_id, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_pnp_client_property_patch_get_publish_topic_NULL_out_span_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_patch_get_publish_topic(
      &client, test_device_request_id, test_buf, 0, &test_length));
}

static void test_az_iot_pnp_client_property_parse_received_topic_NULL_client_fails()
{
  az_iot_pnp_client_property_response response;

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_parse_received_topic(
      NULL, test_property_received_topic_desired_success, &response));
}

static void test_az_iot_pnp_client_property_parse_received_topic_NULL_rec_topic_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  az_iot_pnp_client_property_response response;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_pnp_client_property_parse_received_topic(&client, AZ_SPAN_EMPTY, &response));
}

static void test_az_iot_pnp_client_property_parse_received_topic_NULL_response_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_parse_received_topic(
      &client, test_property_received_topic_desired_success, NULL));
}

static void test_az_iot_pnp_client_property_builder_begin_component_NULL_client_fails()
{
  az_json_writer jw;
  ASSERT_PRECONDITION_CHECKED(
      az_iot_pnp_client_property_builder_begin_component(NULL, &jw, test_component_one));
}

static void test_az_iot_pnp_client_property_builder_begin_component_NULL_jw_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  ASSERT_PRECONDITION_CHECKED(
      az_iot_pnp_client_property_builder_begin_component(&client, NULL, test_component_one));
}

static void test_az_iot_pnp_client_property_builder_begin_component_NULL_component_name_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  az_json_writer jw;
  ASSERT_PRECONDITION_CHECKED(
      az_iot_pnp_client_property_builder_begin_component(&client, &jw, AZ_SPAN_EMPTY));
}

static void test_az_iot_pnp_client_property_builder_end_component_NULL_client_fails()
{
  az_json_writer jw;
  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_builder_end_component(NULL, &jw));
}

static void test_az_iot_pnp_client_property_builder_end_component_NULL_jw_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_pnp_client_property_builder_end_component(&client, NULL));
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_iot_pnp_client_property_document_get_publish_topic_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_pnp_client_property_document_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_property_get_request_topic, test_buf);
  assert_int_equal(sizeof(test_correct_property_get_request_topic) - 1, test_length);
}

static void test_az_iot_pnp_client_property_document_get_publish_topic_small_buffer_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[_az_COUNTOF(test_correct_property_get_request_topic) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_pnp_client_property_document_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_pnp_client_property_patch_get_publish_topic_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_pnp_client_property_patch_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_property_patch_pub_topic, test_buf);
  assert_int_equal(sizeof(test_correct_property_patch_pub_topic) - 1, test_length);
}

static void test_az_iot_pnp_client_property_patch_get_publish_topic_small_buffer_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);

  char test_buf[_az_COUNTOF(test_correct_property_patch_pub_topic) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_pnp_client_property_patch_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_pnp_client_property_parse_received_topic_desired_found_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_iot_pnp_client_property_response response;

  assert_int_equal(
      az_iot_pnp_client_property_parse_received_topic(
          &client, test_property_received_topic_desired_success, &response),
      AZ_OK);
  assert_true(az_span_is_content_equal((response.version), test_device_request_id));
  assert_true(az_span_is_content_equal(response.request_id, AZ_SPAN_EMPTY));
  assert_int_equal(response.status, AZ_IOT_STATUS_OK);
  assert_int_equal(
      response.response_type, AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES);
}

static void test_az_iot_pnp_client_property_parse_received_topic_get_response_found_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_iot_pnp_client_property_response response;

  assert_int_equal(
      az_iot_pnp_client_property_parse_received_topic(
          &client, test_property_received_get_response, &response),
      AZ_OK);
  assert_true(az_span_is_content_equal(response.request_id, test_device_request_id));
  assert_true(az_span_is_content_equal(response.version, AZ_SPAN_EMPTY));
  assert_int_equal(response.status, AZ_IOT_STATUS_OK);
  assert_int_equal(response.response_type, AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET);
}

static void test_az_iot_pnp_client_property_parse_received_topic_reported_props_found_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_iot_pnp_client_property_response response;

  assert_int_equal(
      az_iot_pnp_client_property_parse_received_topic(
          &client, test_property_reported_props_success_response, &response),
      AZ_OK);
  assert_true(az_span_is_content_equal(response.request_id, test_device_request_id));
  assert_true(az_span_is_content_equal(response.version, AZ_SPAN_FROM_STR("16")));
  assert_int_equal(response.status, AZ_IOT_STATUS_NO_CONTENT);
  assert_int_equal(
      response.response_type, AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_REPORTED_PROPERTIES);
}

static void test_az_iot_pnp_client_property_parse_received_topic_not_found_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_iot_pnp_client_property_response response;

  assert_int_equal(
      az_iot_pnp_client_property_parse_received_topic(
          &client, test_property_received_topic_fail, &response),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_pnp_client_property_parse_received_topic_not_found_prefix_fails()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_iot_pnp_client_property_response response;

  assert_int_equal(
      az_iot_pnp_client_property_parse_received_topic(
          &client, test_property_received_topic_prefix_fail, &response),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_pnp_client_property_builder_begin_component_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[100] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);
  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_component(&client, &jw, test_component_one), AZ_OK);
  assert_string_equal(json_buffer, "{\"component_one\":{\"__t\":\"c\"");
}

static void test_az_iot_pnp_client_property_builder_end_component_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[100] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);
  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_component(&client, &jw, test_component_one), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_component(&client, &jw), AZ_OK);
  assert_string_equal(json_buffer, "{\"component_one\":{\"__t\":\"c\"}");
}

static void test_az_iot_pnp_client_property_builder_end_component_with_user_data_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[100] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);
  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_component(&client, &jw, test_component_one), AZ_OK);
  assert_int_equal(az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR("prop")), AZ_OK);
  assert_int_equal(az_json_writer_append_int32(&jw, 100), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_component(&client, &jw), AZ_OK);
  assert_string_equal(json_buffer, "{\"component_one\":{\"__t\":\"c\",\"prop\":100}");
}

static void test_az_iot_pnp_client_property_builder_begin_reported_status_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[128] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);
  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_reported_status(
          &client,
          &jw,
          AZ_SPAN_FROM_STR("targetTemperature"),
          200,
          29,
          AZ_SPAN_FROM_STR("success")),
      AZ_OK);

  assert_string_equal(
      json_buffer, "{\"targetTemperature\":{\"ac\":200,\"av\":29,\"ad\":\"success\",\"value\":");
}

static void test_az_iot_pnp_client_property_builder_end_reported_status_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[128] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);
  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_reported_status(
          &client,
          &jw,
          AZ_SPAN_FROM_STR("targetTemperature"),
          200,
          29,
          AZ_SPAN_FROM_STR("success")),
      AZ_OK);
  assert_int_equal(az_json_writer_append_int32(&jw, 50), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_reported_status(&client, &jw), AZ_OK);

  assert_int_equal(az_json_writer_append_end_object(&jw), AZ_OK);

  assert_string_equal(
      json_buffer,
      "{\"targetTemperature\":{\"ac\":200,\"av\":29,\"ad\":\"success\",\"value\":50}}");
}

static void test_az_iot_pnp_client_property_builder_begin_reported_status_with_component_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[128] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);

  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);
  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_component(
          &client, &jw, AZ_SPAN_FROM_STR("component_one")),
      AZ_OK);
  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_reported_status(
          &client, &jw, AZ_SPAN_FROM_STR("targetTemperature"), 200, 5, AZ_SPAN_FROM_STR("success")),
      AZ_OK);
  assert_int_equal(az_json_writer_append_int32(&jw, 23), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_reported_status(&client, &jw), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_component(&client, &jw), AZ_OK);

  assert_int_equal(az_json_writer_append_end_object(&jw), AZ_OK);

  assert_string_equal(
      json_buffer,
      "{\"component_one\":{\"__t\":\"c\",\"targetTemperature\":{\"ac\":200,\"av\":5,"
      "\"ad\":\"success\",\"value\":23}}}");
}

static void
test_az_iot_pnp_client_property_builder_begin_reported_status_with_component_multiple_values_succeed()
{
  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL),
      AZ_OK);
  az_json_writer jw;
  char json_buffer[256] = { 0 };
  assert_int_equal(az_json_writer_init(&jw, AZ_SPAN_FROM_BUFFER(json_buffer), NULL), AZ_OK);

  assert_int_equal(az_json_writer_append_begin_object(&jw), AZ_OK);
  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_component(
          &client, &jw, AZ_SPAN_FROM_STR("component_one")),
      AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_reported_status(
          &client, &jw, AZ_SPAN_FROM_STR("targetTemperature"), 200, 5, AZ_SPAN_FROM_STR("success")),
      AZ_OK);
  assert_int_equal(az_json_writer_append_int32(&jw, 23), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_reported_status(&client, &jw), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_builder_begin_reported_status(
          &client, &jw, AZ_SPAN_FROM_STR("targetHumidity"), 200, 8, AZ_SPAN_FROM_STR("success")),
      AZ_OK);
  assert_int_equal(az_json_writer_append_int32(&jw, 95), AZ_OK);
  assert_int_equal(az_iot_pnp_client_property_builder_end_reported_status(&client, &jw), AZ_OK);

  assert_int_equal(az_iot_pnp_client_property_builder_end_component(&client, &jw), AZ_OK);
  assert_int_equal(az_json_writer_append_end_object(&jw), AZ_OK);

  assert_string_equal(
      json_buffer,
      "{\"component_one\":{\"__t\":\"c\",\"targetTemperature\":{\"ac\":200,\"av\":5,\"ad\":"
      "\"success\",\"value\":23},\"targetHumidity\":{\"ac\":200,\"av\":8,\"ad\":\"success\","
      "\"value\":95}}}");
}

static bool _should_write_any_mqtt(az_log_classification classification)
{
  switch (classification)
  {
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
    case AZ_LOG_MQTT_RECEIVED_PAYLOAD:
      return true;
    default:
      return false;
  }
}

static int _log_invoked_topic = 0;
static void _log_listener(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
      assert_memory_equal(
          az_span_ptr(test_property_received_topic_desired_success),
          az_span_ptr(message),
          (size_t)az_span_size(message));
      _log_invoked_topic++;
      break;
    default:
      assert_true(false);
  }
}

static void test_az_iot_pnp_client_property_logging_succeed()
{
  az_log_set_message_callback(_log_listener);
  az_log_set_classification_filter_callback(_should_write_any_mqtt);

  assert_int_equal(0, _log_invoked_topic);

  _log_invoked_topic = 0;

  az_iot_pnp_client client;
  assert_true(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_model_id, NULL)
      == AZ_OK);

  az_iot_pnp_client_property_response response;
  assert_int_equal(
      az_iot_pnp_client_property_parse_received_topic(
          &client, test_property_received_topic_desired_success, &response),
      AZ_OK);

  assert_int_equal(_az_BUILT_WITH_LOGGING(1, 0), _log_invoked_topic);

  az_log_set_message_callback(NULL);
  az_log_set_classification_filter_callback(NULL);
}

static void test_az_iot_pnp_client_property_get_property_version_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES;
  az_json_reader jr;
  int32_t version;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload, NULL), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_property_version(&client, &jr, response_type, &version),
      AZ_OK);
  assert_int_equal(version, 5);
}

static void test_az_iot_pnp_client_property_get_property_version_long_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET;
  az_json_reader jr;
  int32_t version;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_long, NULL), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_property_version(&client, &jr, response_type, &version),
      AZ_OK);
  assert_int_equal(version, 30);
}

static void test_az_iot_pnp_client_property_get_property_version_out_of_order_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET;
  az_json_reader jr;
  int32_t version;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_out_of_order, NULL), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_property_version(&client, &jr, response_type, &version),
      AZ_OK);
  assert_int_equal(version, 4);
}

static void test_az_iot_pnp_client_property_get_next_component_property_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES;
  az_span component_name;
  int32_t value;

  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_one")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 1);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_two")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("string")));

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Second component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_two));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_three")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 45);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_four")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("string")));

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Not a component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, AZ_SPAN_EMPTY));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("not_component")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 42);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // End of components (skipping version)
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_IOT_END_OF_PROPERTIES);
}

static void test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_fail()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES;
  az_span component_name;
  int32_t value;

  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_one")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 1);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_JSON_INVALID_STATE);
}

static void
test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_in_object_fail()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_with_user_object, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES;
  az_span component_name;
  int32_t value;

  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_one")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 1);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_two")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Begin user object
  assert_true(jr.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT);
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // At user property name
  assert_true(jr.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_JSON_INVALID_STATE);
}

static void
test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_in_top_level_object_fail()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_with_user_object, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES;
  az_span component_name;
  int32_t value;

  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_one")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 1);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_two")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Skip over user object
  assert_int_equal(az_json_reader_skip_children(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);

  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_two));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_three")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 45);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_two));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_four")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("string")));

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Non component property
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, AZ_SPAN_EMPTY));

  // Advance to user object
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_true(jr.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT);

  // Advance to non-component property name
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_JSON_INVALID_STATE);
}

static void test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_root_fail()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_components;
  options.component_names_length = test_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES;
  az_span component_name;
  int32_t value;

  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_one")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 1);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_two")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("string")));

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Second component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_component_two));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_three")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 45);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("prop_four")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("string")));

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Not a component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, AZ_SPAN_EMPTY));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("not_component")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 42);

  // End of components (skipping version)
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_JSON_INVALID_STATE);
}

static void test_az_iot_pnp_client_property_get_next_component_property_two_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_temperature_components;
  options.component_names_length = test_temperature_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_two, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET;
  az_span component_name;
  int32_t value;
  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_temp_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 47);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // End of components (skipping version and reported properties section)
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_IOT_END_OF_PROPERTIES);
}

static void test_az_iot_pnp_client_property_get_next_component_property_out_of_order_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_temperature_components;
  options.component_names_length = test_temperature_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_out_of_order, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET;
  az_span component_name;
  int32_t value;
  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_temp_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 47);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // End of components (skipping version and reported properties section)
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_IOT_END_OF_PROPERTIES);
}

static void test_az_iot_pnp_client_property_get_next_component_property_long_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_temperature_components;
  options.component_names_length = test_temperature_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_long, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET;
  az_span component_name;
  int32_t value;
  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_temp_component_two));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 50);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Second component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_temp_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 90);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Not a component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, AZ_SPAN_EMPTY));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 54);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // End of components (skipping version and reported properties section)
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_IOT_END_OF_PROPERTIES);
}

static void test_az_iot_pnp_client_property_get_next_component_property_long_with_version_succeed()
{
  az_iot_pnp_client client;
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = test_temperature_components;
  options.component_names_length = test_temperature_components_length;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_model_id, &options),
      AZ_OK);

  az_json_reader jr;
  assert_int_equal(az_json_reader_init(&jr, test_property_payload_long, NULL), AZ_OK);

  az_iot_pnp_client_property_response_type response_type
      = AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET;
  int32_t version;
  assert_int_equal(
      az_iot_pnp_client_property_get_property_version(&client, &jr, response_type, &version),
      AZ_OK);
  assert_int_equal(version, 30);

  assert_int_equal(az_json_reader_init(&jr, test_property_payload_long, NULL), AZ_OK);

  az_span component_name;
  int32_t value;
  // First component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_temp_component_two));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 50);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Second component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, test_temp_component_one));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 90);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // Not a component
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_OK);
  assert_true(az_span_is_content_equal(component_name, AZ_SPAN_EMPTY));
  assert_true(az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("targetTemperature")));
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_token_get_int32(&jr.token, &value), AZ_OK);
  assert_int_equal(value, 54);

  // Advance
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  // End of components (skipping version and reported properties section)
  assert_int_equal(
      az_iot_pnp_client_property_get_next_component_property(
          &client, &jr, response_type, &component_name),
      AZ_ERROR_IOT_END_OF_PROPERTIES);
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_pnp_client_property()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_pnp_client_property_document_get_publish_topic_NULL_client_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_document_get_publish_topic_NULL_request_id_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_document_get_publish_topic_NULL_span_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_document_get_publish_topic_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_patch_get_publish_topic_NULL_client_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_patch_get_publish_topic_invalid_request_id_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_patch_get_publish_topic_NULL_char_buf_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_patch_get_publish_topic_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_parse_received_topic_NULL_client_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_parse_received_topic_NULL_rec_topic_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_parse_received_topic_NULL_response_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_begin_component_NULL_client_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_begin_component_NULL_jw_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_builder_begin_component_NULL_component_name_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_end_component_NULL_client_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_end_component_NULL_jw_fails),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_pnp_client_property_document_get_publish_topic_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_document_get_publish_topic_small_buffer_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_patch_get_publish_topic_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_patch_get_publish_topic_small_buffer_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_parse_received_topic_desired_found_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_parse_received_topic_get_response_found_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_parse_received_topic_reported_props_found_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_parse_received_topic_not_found_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_parse_received_topic_not_found_prefix_fails),
    cmocka_unit_test(test_az_iot_pnp_client_property_logging_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_begin_component_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_end_component_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_end_component_with_user_data_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_get_property_version_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_get_property_version_long_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_get_property_version_out_of_order_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_get_next_component_property_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_fail),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_in_object_fail),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_in_top_level_object_fail),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_get_next_component_property_user_not_advance_root_fail),
    cmocka_unit_test(test_az_iot_pnp_client_property_get_next_component_property_two_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_get_next_component_property_long_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_get_next_component_property_out_of_order_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_get_next_component_property_long_with_version_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_begin_reported_status_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_builder_begin_reported_status_with_component_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_property_builder_begin_reported_status_with_component_multiple_values_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_property_builder_end_reported_status_succeed),
  };

  return cmocka_run_group_tests_name("az_iot_pnp_client_property", tests, NULL, NULL);
}
