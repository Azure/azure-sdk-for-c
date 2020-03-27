// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_pnp_client.h"
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

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_ROOT_INTERFACE_NAME "my_root_interface_name"
#define TEST_CONTENT_TYPE "my_content_type"
#define TEST_CONTENT_ENCODING "my_content_encoding"

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR("my_device");
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR("myiothub.azure-devices.net");

#ifndef NO_PRECONDITION_CHECKING

enable_precondition_check_tests()

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_pnp_client_telemetry_publish_topic_get_no_options_no_props_succeed(
    void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
        &client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  uint8_t mqtt_topic_buf[_az_COUNTOF(g_test_correct_topic_no_options_no_props) - 1];
  az_span mqtt_topic = az_span_init(mqtt_topic_buf, 0, _az_COUNTOF(mqtt_topic_buf));

  assert_int_equal(
      az_iot_hub_client_telemetry_publish_topic_get(&client, NULL, mqtt_topic, &mqtt_topic), AZ_OK);
  assert_memory_equal(
      g_test_correct_topic_no_options_no_props,
      (char*)az_span_ptr(mqtt_topic),
      _az_COUNTOF(g_test_correct_topic_no_options_no_props) - 1);
}



int test_iot_pnp_telemetry()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  return 0;
}
