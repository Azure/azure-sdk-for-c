// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_span.h>

#include <az_precondition_internal.h>
#include <az_precondition.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <az_test_precondition.h>

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"

static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);

static const char g_test_correct_subscribe_topic[] = "devices/my_device/messages/devicebound/#";


#ifndef NO_PRECONDITION_CHECKING

enable_precondition_check_tests()

static void test_az_iot_hub_client_c2d_received_topic_parse_NULL_client_fail(void** state)
{
  (void)state;

  az_span received_topic = AZ_SPAN_FROM_STR("devices/useragent_c/messages/devicebound/$.mid=79eadb01-bd0d-472d-bd35-ccb76e70eab8&$.to=/devices/useragent_c/messages/deviceBound&iothub-ack=full");

  az_iot_hub_client_c2d_request out_request;

  assert_precondition_checked(
    az_iot_hub_client_c2d_received_topic_parse(NULL, received_topic, &out_request)
  );
}

static void test_az_iot_hub_client_c2d_received_topic_parse_AZ_SPAN_NULL_received_topic_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_NULL;

  az_iot_hub_client_c2d_request out_request;

  assert_precondition_checked(
    az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, &out_request)
  );
}

static void test_az_iot_hub_client_c2d_received_topic_parse_NULL_out_request_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("a/b/c=1&d=2");

  assert_precondition_checked(
    az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, NULL)
  );
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_c2d_subscribe_topic_filter_get_succeed(void** state)
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

static void test_az_iot_hub_client_c2d_subscribe_topic_filter_get_small_buffer_fail(void** state)
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

static void test_az_iot_hub_client_c2d_received_topic_parse_succeed(void** state)
{
  (void)state;

  // az_pair pair;
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("devices/useragent_c/messages/devicebound/$.mid=79eadb01-bd0d-472d-bd35-ccb76e70eab8&$.to=/devices/useragent_c/messages/deviceBound&abc=123");

  az_iot_hub_client_c2d_request out_request;

  assert_return_code(az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, &out_request), AZ_OK);

  // TODO: enable after az_iot_hub_client_properties_next() is implemented.
  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("$.mid")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("79eadb01-bd0d-472d-bd35-ccb76e70eab8")));

  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("$.to")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("/devices/useragent_c/messages/deviceBound")));

  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("abc")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("123")));
}


int test_iot_hub_c2d()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_NULL_client_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_AZ_SPAN_NULL_received_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_NULL_out_request_fail),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_c2d_subscribe_topic_filter_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_subscribe_topic_filter_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_succeed)
  };
  return cmocka_run_group_tests_name("az_iot_hub_c2d", tests, NULL, NULL);
}
