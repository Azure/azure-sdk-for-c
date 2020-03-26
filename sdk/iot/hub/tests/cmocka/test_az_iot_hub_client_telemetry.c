// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_span.h>

#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

#define TEST_MQTT_SPAN_BUFFER_SIZE 128

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR("my_device");
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR("myiothub.azure-devices.net");
static const az_span test_module_id = AZ_SPAN_LITERAL_FROM_STR("my_module_id");
static const az_span test_props = AZ_SPAN_LITERAL_FROM_STR("key=value&key_two=value2");

static const char g_test_correct_topic_no_options_no_props[] = "devices/my_device/messages/events/";
static const char g_test_correct_topic_with_options_no_props[]
    = "devices/my_device/modules/my_module_id/messages/events/";
static const char g_test_correct_topic_with_options_with_props[]
    = "devices/my_device/modules/my_module_id/messages/events/"
      "key=value&key_two=value2";
static const char g_test_correct_topic_no_options_with_props[]
    = "devices/my_device/messages/events/"
      "key=value&key_two=value2";
static const char g_test_correct_topic_with_options_module_id_with_props[]
    = "devices/my_device/modules/my_module_id/messages/events/key=value&key_two=value2";

#ifndef NO_PRECONDITION_CHECKING

enable_precondition_check_tests()

    static void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_precondition_checked(
      az_iot_hub_client_telemetry_publish_topic_get(NULL, NULL, mqtt_topic, &mqtt_topic));
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_mqtt_topic_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_buf[1];
  az_span bad_mqtt_topic = az_span_init(test_buf, 0, _az_COUNTOF(test_buf));
  bad_mqtt_topic._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_telemetry_publish_topic_get(
      &client, NULL, bad_mqtt_topic, &bad_mqtt_topic));
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_out_mqtt_topic_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_precondition_checked(
      az_iot_hub_client_telemetry_publish_topic_get(&client, NULL, mqtt_topic, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_no_props_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];
  memset(mqtt_topic_buf, 0xFF, _az_COUNTOF(mqtt_topic_buf));
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, NULL, mqtt_topic, &mqtt_topic), AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_no_options_no_props,
      (char*)az_span_ptr(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_no_options_no_props) - 1);
  assert_int_equal(
      az_span_length(mqtt_topic), _az_COUNTOF(g_test_correct_topic_no_options_no_props) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), TEST_MQTT_SPAN_BUFFER_SIZE);
  assert_int_equal(mqtt_topic_buf[az_span_length(mqtt_topic)], 0xFF);
}

static void
test_az_iot_hub_client_telemetry_publish_topic_get_as_string_no_options_no_props_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE] = { 0 };
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_true(
      az_iot_hub_client_telemetry_publish_topic_get(&client, NULL, mqtt_topic, &mqtt_topic)
      == AZ_OK);
  assert_int_equal(
      az_span_length(mqtt_topic), _az_COUNTOF(g_test_correct_topic_no_options_no_props) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), TEST_MQTT_SPAN_BUFFER_SIZE);
  assert_string_equal(g_test_correct_topic_no_options_no_props, (char*)az_span_ptr(mqtt_topic));
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_no_props_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = test_module_id;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];
  memset(mqtt_topic_buf, 0xFF, _az_COUNTOF(mqtt_topic_buf));
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, NULL, mqtt_topic, &mqtt_topic), AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_with_options_no_props,
      (char*)az_span_ptr(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_with_options_no_props) - 1);
  assert_int_equal(
      az_span_length(mqtt_topic), _az_COUNTOF(g_test_correct_topic_with_options_no_props) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), TEST_MQTT_SPAN_BUFFER_SIZE);
  assert_int_equal(mqtt_topic_buf[az_span_length(mqtt_topic)], 0xFF);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_props_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = test_module_id;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_props), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];
  memset(mqtt_topic_buf, 0xFF, _az_COUNTOF(mqtt_topic_buf));
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, &props, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_with_options_with_props,
      (char*)az_span_ptr(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_with_options_with_props) - 1);
  assert_int_equal(
      az_span_length(mqtt_topic), _az_COUNTOF(g_test_correct_topic_with_options_with_props) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), TEST_MQTT_SPAN_BUFFER_SIZE);
  assert_int_equal(mqtt_topic_buf[az_span_length(mqtt_topic)], 0xFF);
}

static void
test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_props_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = test_module_id;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_props), AZ_OK);

  uint8_t mqtt_topic_buf[_az_COUNTOF(g_test_correct_topic_with_options_with_props) - 2];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, &props, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(mqtt_topic), 0);
  assert_int_equal(
      az_span_capacity(mqtt_topic), _az_COUNTOF(g_test_correct_topic_with_options_with_props) - 2);
}

static void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_props_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_props), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];
  memset(mqtt_topic_buf, 0xFF, _az_COUNTOF(mqtt_topic_buf));
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, &props, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_no_options_with_props,
      (char*)az_span_ptr(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_no_options_with_props) - 1);
  assert_int_equal(
      az_span_length(mqtt_topic), _az_COUNTOF(g_test_correct_topic_no_options_with_props) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), TEST_MQTT_SPAN_BUFFER_SIZE);
  assert_int_equal(mqtt_topic_buf[az_span_length(mqtt_topic)], 0xFF);
}

static void
test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_props_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_props), AZ_OK);

  uint8_t mqtt_topic_buf[_az_COUNTOF(g_test_correct_topic_no_options_with_props) - 2];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, &props, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(mqtt_topic), 0);
  assert_int_equal(
      az_span_capacity(mqtt_topic), _az_COUNTOF(g_test_correct_topic_no_options_with_props) - 2);
}

static void
test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_props_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = test_module_id;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_props), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_MQTT_SPAN_BUFFER_SIZE];
  memset(mqtt_topic_buf, 0xFF, _az_COUNTOF(mqtt_topic_buf));
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, &props, mqtt_topic, &mqtt_topic),
      AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_with_options_module_id_with_props,
      (char*)az_span_ptr(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_with_options_module_id_with_props) - 1);
  assert_int_equal(
      az_span_length(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_with_options_module_id_with_props) - 1);
  assert_int_equal(az_span_capacity(mqtt_topic), TEST_MQTT_SPAN_BUFFER_SIZE);
  assert_int_equal(mqtt_topic_buf[az_span_length(mqtt_topic)], 0xFF);
}

static void
test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_props_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = test_module_id;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_props), AZ_OK);

  uint8_t mqtt_topic_buf[_az_COUNTOF(g_test_correct_topic_with_options_module_id_with_props) - 2];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, &props, mqtt_topic, &mqtt_topic),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(mqtt_topic), 0);
  assert_int_equal(
      az_span_capacity(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_with_options_module_id_with_props) - 2);
}

int test_iot_hub_telemetry()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_NULL_mqtt_topic_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_NULL_out_mqtt_topic_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_no_options_no_props_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_as_string_no_options_no_props_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_no_props_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_props_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_props_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_props_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_props_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_props_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_props_small_buffer_fails),
  };

  return cmocka_run_group_tests_name("az_iot_hub_client_telemetry", tests, NULL, NULL);
}
