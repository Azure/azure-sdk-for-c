// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_hub_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"

static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);

static const char g_test_correct_subscribe_topic[] = "devices/my_device/messages/devicebound/#";

void test_az_iot_hub_client_c2d_subscribe_topic_filter_get_succeed(void** state)
{
  (void)state;

  uint8_t mqtt_sub_topic_buf[sizeof(g_test_correct_subscribe_topic)];
  az_span mqtt_sub_topic = az_span_init(mqtt_sub_topic_buf, 0, _az_COUNTOF(mqtt_sub_topic_buf));

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  assert_true(
      az_iot_hub_client_c2d_subscribe_topic_filter_get(&client, mqtt_sub_topic, &mqtt_sub_topic)
      == AZ_OK);
  assert_string_equal(g_test_correct_subscribe_topic, az_span_ptr(mqtt_sub_topic));
}

void test_az_iot_hub_client_c2d_subscribe_topic_filter_get_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t mqtt_sub_topic_buf[sizeof(g_test_correct_subscribe_topic) - 1];
  az_span mqtt_sub_topic = az_span_init(mqtt_sub_topic_buf, 0, _az_COUNTOF(mqtt_sub_topic_buf));

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  assert_true(
      az_iot_hub_client_c2d_subscribe_topic_filter_get(&client, mqtt_sub_topic, &mqtt_sub_topic)
      == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

int test_iot_hub_c2d()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_hub_client_c2d_subscribe_topic_filter_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_subscribe_topic_filter_get_small_buffer_fail),
  };
  return cmocka_run_group_tests_name("az_iot_hub_c2d", tests, NULL, NULL);
}
