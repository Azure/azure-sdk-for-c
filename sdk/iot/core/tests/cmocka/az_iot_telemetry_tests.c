// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_iot_tests.h"
#include <az_iot_hub_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#define TEST_FQDN "myiothub.azure-devices.net"
#define TEST_DEVICE_ID "my_device"
#define TEST_MODULE_ID "my_module_id"
#define TEST_USER_AGENT "api-version=2017-11-08-preview&DeviceClientType=iothubclient"
#define TEST_PARAMS "key=value&key_two=value2"
#define TEST_MQTT_SPAN_BUFFER_SIZE 100

#define TEST_BAD_SPAN \
  { \
    ._internal = { .ptr = (uint8_t*)0x7FFFFFFF, .length = -1, .capacity = 10 }, \
  }
#define TEST_EMPTY_OPTIONS \
  { \
    .module_id = AZ_SPAN_LITERAL_NULL, .user_agent = AZ_SPAN_LITERAL_NULL \
  }
#define TEST_VALID_OPTIONS_BOTH \
  { \
    .module_id = AZ_SPAN_LITERAL_FROM_STR(TEST_MODULE_ID), \
    .user_agent = AZ_SPAN_LITERAL_FROM_STR(TEST_USER_AGENT) \
  }
#define TEST_VALID_OPTIONS_MODULE_ID \
  { \
    .module_id = AZ_SPAN_LITERAL_FROM_STR(TEST_MODULE_ID), .user_agent = AZ_SPAN_LITERAL_NULL \
  }
#define TEST_VALID_OPTIONS_USER_AGENT \
  { \
    .module_id = AZ_SPAN_LITERAL_NULL, .user_agent = AZ_SPAN_LITERAL_FROM_STR(TEST_USER_AGENT) \
  }

static const az_iot_hub_client_properties g_test_params
    = { ._internal
        = { .properties = AZ_SPAN_LITERAL_FROM_STR(TEST_PARAMS), .current_property = 0 } };

static const char g_test_correct_topic_no_options_no_params[]
    = "devices/my_device/messages/events/";
static const char g_test_correct_topic_with_options_no_params[]
    = "devices/my_device/modules/my_module_id/messages/events/"
      "?api-version=2017-11-08-preview&DeviceClientType=iothubclient";
static const char g_test_correct_topic_with_options_with_params[]
    = "devices/my_device/modules/my_module_id/messages/events/"
      "?key=value&key_two=value2&api-version=2017-11-08-preview&DeviceClientType=iothubclient";
static const char g_test_correct_topic_no_options_with_params[]
    = "devices/my_device/messages/events/"
      "?key=value&key_two=value2";
static const char g_test_correct_topic_with_options_module_id_with_params[]
    = "devices/my_device/modules/my_module_id/messages/events/?key=value&key_two=value2";
static const char g_test_correct_topic_with_options_user_agent_with_params[]
    = "devices/my_device/messages/events/"
      "?key=value&key_two=value2&api-version=2017-11-08-preview&DeviceClientType=iothubclient";

static const az_iot_hub_client g_test_valid_client_no_options
    = { ._internal = { .iot_hub_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_FQDN),
                       .device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID),
                       .options = TEST_EMPTY_OPTIONS } };
static const az_iot_hub_client g_test_valid_client_with_options_both
    = { ._internal = { .iot_hub_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_FQDN),
                       .device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID),
                       .options = TEST_VALID_OPTIONS_BOTH } };
static const az_iot_hub_client g_test_valid_client_with_options_module_id
    = { ._internal = { .iot_hub_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_FQDN),
                       .device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID),
                       .options = TEST_VALID_OPTIONS_MODULE_ID } };
static const az_iot_hub_client g_test_valid_client_with_options_user_agent
    = { ._internal = { .iot_hub_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_FQDN),
                       .device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID),
                       .options = TEST_VALID_OPTIONS_USER_AGENT } };

/*
static void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(NULL, NULL, mqtt_topic, &mqtt_topic),
      AZ_ERROR_ARG);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_mqtt_topic_fails(void** state)
{
  (void)state;
  az_span null_mqtt_topic = TEST_BAD_SPAN;

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_no_options, NULL, null_mqtt_topic, &null_mqtt_topic),
      AZ_ERROR_ARG);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_out_mqtt_topic_fails(void** state)
{
  (void)state;
  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_no_options, NULL, mqtt_topic, NULL),
      AZ_ERROR_ARG);
}
*/

static void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_no_params_succeed(void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_no_options_no_params) - 1];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_no_options, NULL, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_no_options_no_params,
      (char*)az_span_ptr(mqtt_topic),
      sizeof(g_test_correct_topic_no_options_no_params) - 1);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_as_string_no_options_no_params_succeed(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_no_options_no_params)] = { 0 };

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_true(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_no_options, NULL, mqtt_topic, &mqtt_topic)
      == AZ_OK);
  assert_int_equal(
      az_span_length(mqtt_topic), sizeof(g_test_correct_topic_no_options_no_params) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), sizeof(g_test_correct_topic_no_options_no_params));
  assert_string_equal(g_test_correct_topic_no_options_no_params, (char*)az_span_ptr(mqtt_topic));
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_no_params_succeed(void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_no_params) - 1];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_both, NULL, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_with_options_no_params,
      (char*)az_span_ptr(mqtt_topic),
      sizeof(g_test_correct_topic_with_options_no_params) - 1);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_succeed(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_with_params) - 1];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_both, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_with_options_with_params,
      (char*)az_span_ptr(mqtt_topic),
      sizeof(g_test_correct_topic_with_options_with_params) - 1);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_small_buffer_fails(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_with_params) - 2];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_both, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_succeed(void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_no_options_with_params) - 1];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_no_options, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_no_options_with_params,
      (char*)az_span_ptr(mqtt_topic),
      sizeof(g_test_correct_topic_no_options_with_params) - 1);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_small_buffer_fails(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_no_options_with_params) - 2];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_no_options, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_succeed(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_module_id_with_params) - 1];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_module_id, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_with_options_module_id_with_params,
      (char*)az_span_ptr(mqtt_topic),
      sizeof(g_test_correct_topic_with_options_module_id_with_params) - 1);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_small_buffer_fails(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_module_id_with_params) - 2];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_module_id, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_succeed(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_user_agent_with_params) - 1];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_user_agent, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_OK);

  assert_memory_equal(
      g_test_correct_topic_with_options_user_agent_with_params,
      (char*)az_span_ptr(mqtt_topic),
      sizeof(g_test_correct_topic_with_options_user_agent_with_params) - 1);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_small_buffer_fails(
    void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[sizeof(g_test_correct_topic_with_options_user_agent_with_params) - 2];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(
          &g_test_valid_client_with_options_user_agent, &g_test_params, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

int test_iot_hub_telemetry()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_no_options_no_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_as_string_no_options_no_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_no_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_small_buffer_fails),
  };

  return cmocka_run_group_tests_name("az_iot_telemetry", tests, NULL, NULL);
}
