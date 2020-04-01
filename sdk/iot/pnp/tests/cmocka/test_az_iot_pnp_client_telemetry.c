// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_pnp_client.h"
#include <az_iot_pnp_client.h>
#include <az_test_span.h>

#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 128

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_ROOT_INTERFACE_NAME "my_root_interface_name"
#define TEST_CONTENT_TYPE "my_content_type"
#define TEST_CONTENT_ENCODING "my_content_encoding"
#define TEST_COMPONENT_NAME "my_component_name"

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR("my_device");
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR("myiothub.azure-devices.net");
static const az_span test_root_interface_name = AZ_SPAN_LITERAL_FROM_STR(TEST_ROOT_INTERFACE_NAME);
static const az_span test_component_name = AZ_SPAN_LITERAL_FROM_STR(TEST_COMPONENT_NAME);
static const char g_test_correct_pnp_topic_no_options_no_props[] = "devices/my_device/messages/events/%24.ifname=" TEST_COMPONENT_NAME;

#ifndef NO_PRECONDITION_CHECKING

enable_precondition_check_tests()

    static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];

  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_precondition_checked(
      az_iot_pnp_client_telemetry_get_publish_topic(NULL, test_component_name, mqtt_topic, NULL, &mqtt_topic));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_mqtt_topic_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
        &client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  uint8_t test_buf[1];
  az_span bad_mqtt_topic = az_span_init(test_buf, 0, _az_COUNTOF(test_buf));
  bad_mqtt_topic._internal.ptr = NULL;

  assert_precondition_checked(az_iot_pnp_client_telemetry_get_publish_topic(
      &client, test_component_name, bad_mqtt_topic, NULL, &bad_mqtt_topic));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_out_mqtt_topic_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
        &client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_precondition_checked(
      az_iot_pnp_client_telemetry_get_publish_topic(&client, test_component_name, mqtt_topic, NULL, NULL));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_component_name_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
        &client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_precondition_checked(
      az_iot_pnp_client_telemetry_get_publish_topic(&client, AZ_SPAN_NULL, mqtt_topic, NULL, &mqtt_topic));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_non_NULL_reserved_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
        &client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_precondition_checked(
      az_iot_pnp_client_telemetry_get_publish_topic(&client, test_component_name, AZ_SPAN_NULL, (void*)0x1, &mqtt_topic));
}


#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_pnp_client_telemetry_publish_topic_get_no_options_no_props_succeed(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
        &client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  az_span mqtt_topic = az_span_for_test_init(mqtt_topic_buf, 0, TEST_SPAN_BUFFER_SIZE);

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(&client, test_component_name, mqtt_topic, NULL, &mqtt_topic), AZ_OK);

  az_span_for_test_verify(mqtt_topic, 
      g_test_correct_pnp_topic_no_options_no_props,
      _az_COUNTOF(g_test_correct_pnp_topic_no_options_no_props) - 1);
}

int test_iot_pnp_telemetry()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

    const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
      cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_client_fails),
      cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_mqtt_topic_fails),
      cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_out_mqtt_topic_fails),
      cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_component_name_fails),
      cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_non_NULL_reserved_fails),
#endif // NO_PRECONDITION_CHECKING
      cmocka_unit_test(test_az_iot_pnp_client_telemetry_publish_topic_get_no_options_no_props_succeed),      
    };
    return cmocka_run_group_tests_name("az_iot_pnp_client", tests, NULL, NULL);
}
