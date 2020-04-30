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

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_root_interface_name = AZ_SPAN_LITERAL_FROM_STR(TEST_ROOT_INTERFACE_NAME);
static const az_span test_component_name = AZ_SPAN_LITERAL_FROM_STR(TEST_COMPONENT_NAME);
static const az_span test_content_type = AZ_SPAN_LITERAL_FROM_STR(TEST_CONTENT_TYPE);
static const az_span test_content_encoding = AZ_SPAN_LITERAL_FROM_STR(TEST_CONTENT_ENCODING);

static const char g_test_correct_pnp_topic_no_options[]
    = "devices/my_device/messages/events/%24.ifname=" TEST_COMPONENT_NAME;
static const char g_test_correct_pnp_topic_content_type[]
    = "devices/my_device/messages/events/%24.ifname=" TEST_COMPONENT_NAME
      "&%24.ct=" TEST_CONTENT_TYPE;
static const char g_test_correct_pnp_topic_content_encoding[]
    = "devices/my_device/messages/events/%24.ifname=" TEST_COMPONENT_NAME
      "&%24.ce=" TEST_CONTENT_ENCODING;
static const char g_test_correct_pnp_topic_content_type_and_encoding[]
    = "devices/my_device/messages/events/%24.ifname=" TEST_COMPONENT_NAME
      "&%24.ct=" TEST_CONTENT_TYPE "&%24.ce=" TEST_CONTENT_ENCODING;

#ifndef AZ_NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_client_fails(void** state)
{
  (void)state;

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_precondition_checked(az_iot_pnp_client_telemetry_get_publish_topic(
      NULL, test_component_name, NULL, mqtt_topic_buf, sizeof(mqtt_topic_buf), &mqtt_topic_length));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_mqtt_topic_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_precondition_checked(az_iot_pnp_client_telemetry_get_publish_topic(
      &client, test_component_name, NULL, NULL, sizeof(mqtt_topic_buf), &mqtt_topic_length));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_out_mqtt_topic_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_precondition_checked(az_iot_pnp_client_telemetry_get_publish_topic(
      &client, test_component_name, NULL, mqtt_topic_buf, 0, &mqtt_topic_length));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_component_name_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_precondition_checked(az_iot_pnp_client_telemetry_get_publish_topic(
      &client, AZ_SPAN_NULL, NULL, mqtt_topic_buf, sizeof(mqtt_topic_buf), &mqtt_topic_length));
}

static void test_az_iot_pnp_client_telemetry_get_publish_topic_non_NULL_reserved_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_precondition_checked(az_iot_pnp_client_telemetry_get_publish_topic(
      &client, AZ_SPAN_NULL, (void*)0x1, mqtt_topic_buf, sizeof(mqtt_topic_buf), &mqtt_topic_length));
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_iot_pnp_client_telemetry_publish_topic_get_no_options_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client, test_component_name, NULL, mqtt_topic_buf, sizeof(mqtt_topic_buf), &mqtt_topic_length),
      AZ_OK);

  assert_string_equal(g_test_correct_pnp_topic_no_options, mqtt_topic_buf);
  assert_int_equal(sizeof(g_test_correct_pnp_topic_no_options) - 1, mqtt_topic_length);
}

static void test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.content_type = test_content_type;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_OK);

  assert_string_equal(g_test_correct_pnp_topic_content_type, mqtt_topic_buf);
  assert_int_equal(sizeof(g_test_correct_pnp_topic_content_type) - 1, mqtt_topic_length);
}

static void test_az_iot_pnp_client_telemetry_publish_topic_get_content_encoding_succeed(
    void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.content_encoding = test_content_encoding;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_OK);

  assert_string_equal(g_test_correct_pnp_topic_content_encoding, mqtt_topic_buf);
  assert_int_equal(sizeof(g_test_correct_pnp_topic_content_encoding) - 1, mqtt_topic_length);
}

static void test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_and_encoding_succeed(
    void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.content_type = test_content_type;
  options.content_encoding = test_content_encoding;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_OK);

  assert_string_equal(g_test_correct_pnp_topic_content_type_and_encoding, mqtt_topic_buf);
  assert_int_equal(sizeof(g_test_correct_pnp_topic_content_type_and_encoding) - 1, mqtt_topic_length);
}

static void test_az_iot_pnp_client_telemetry_publish_topic_get_with_small_buffer_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  char mqtt_topic_buf[_az_COUNTOF(g_test_correct_pnp_topic_no_options) - 2];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_with_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.content_type = test_content_type;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  char mqtt_topic_buf[_az_COUNTOF(g_test_correct_pnp_topic_content_type) - 2];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void
test_az_iot_pnp_client_telemetry_publish_topic_get_content_encoding_with_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.content_encoding = test_content_encoding;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  char mqtt_topic_buf[_az_COUNTOF(g_test_correct_pnp_topic_content_type) - 2];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void
test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_and_encoding_with_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.content_type = test_content_type;
  options.content_encoding = test_content_encoding;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  char mqtt_topic_buf[_az_COUNTOF(g_test_correct_pnp_topic_content_type_and_encoding) - 2];
  size_t mqtt_topic_length;

  assert_int_equal(
      az_iot_pnp_client_telemetry_get_publish_topic(
          &client,
          test_component_name,
          NULL,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &mqtt_topic_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

int test_iot_pnp_telemetry()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_client_fails),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_mqtt_topic_fails),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_out_mqtt_topic_fails),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_NULL_component_name_fails),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_get_publish_topic_non_NULL_reserved_fails),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_publish_topic_get_no_options_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_publish_topic_get_content_encoding_succeed),
    cmocka_unit_test(
        test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_and_encoding_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_telemetry_publish_topic_get_with_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_with_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_telemetry_publish_topic_get_content_encoding_with_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_pnp_client_telemetry_publish_topic_get_content_type_and_encoding_with_small_buffer_fails),
  };
  return cmocka_run_group_tests_name("az_iot_pnp_client_telemetry", tests, NULL, NULL);
}
