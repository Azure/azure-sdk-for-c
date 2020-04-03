// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_span.h>
#include <az_test_span.h>

#include <az_precondition_internal.h>
#include <az_precondition.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <az_test_precondition.h>

#define TEST_SPAN_BUFFER_SIZE 128

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"

static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static char g_test_correct_subscribe_topic[] = "devices/my_device/messages/devicebound/#";
static const az_span test_URL_DECODED_topic = AZ_SPAN_LITERAL_FROM_STR("devices/useragent_c/messages/devicebound/$.mid=79eadb01-bd0d-472d-bd35-ccb76e70eab8&$.to=/devices/useragent_c/messages/deviceBound&abc=123");
static const az_span test_URL_ENCODED_topic = AZ_SPAN_LITERAL_FROM_STR("devices/useragent_c/messages/devicebound/%24.to=%2Fdevices%2Fuseragent_c%2Fmessages%2FdeviceBound&abc=123&ghi=%2Fsome%2Fthing&jkl=%2Fsome%2Fthing%2F%3Fbla%3Dbla");


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

  az_span received_topic = test_URL_DECODED_topic;

  assert_precondition_checked(
    az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, NULL)
  );
}

// Note: c2d messages ALWAYS contain propeties (at least $.to).
static void test_az_iot_hub_client_c2d_received_topic_parse_no_properties_fail(void** state)
{
  (void)state;

  // az_pair pair;
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("devices/useragent_c/messages/devicebound/");

  az_iot_hub_client_c2d_request out_request;

  assert_precondition_checked(
    az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, &out_request)
  );
}

static void test_az_iot_hub_client_c2d_received_topic_parse_MALFORMED_fail(void** state)
{
  (void)state;

  // az_pair pair;
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("devices/useragent_c/message#$vicebound/a=1");

  az_iot_hub_client_c2d_request out_request;

  assert_precondition_checked(
    az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, &out_request)
  );
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_c2d_subscribe_topic_filter_get_succeed(void** state)
{
  (void)state;

  uint8_t mqtt_sub_topic_buf[TEST_SPAN_BUFFER_SIZE];
  az_span mqtt_sub_topic = az_span_for_test_init(mqtt_sub_topic_buf, 0, _az_COUNTOF(mqtt_sub_topic_buf));

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  assert_true(
      az_iot_hub_client_c2d_subscribe_topic_filter_get(&client, mqtt_sub_topic, &mqtt_sub_topic)
      == AZ_OK);

  az_span_for_test_verify(
      mqtt_sub_topic,
      g_test_correct_subscribe_topic,
      _az_COUNTOF(g_test_correct_subscribe_topic) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_c2d_subscribe_topic_filter_get_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t mqtt_sub_topic_buf[_az_COUNTOF(g_test_correct_subscribe_topic) - 2];
  az_span mqtt_sub_topic = az_span_init(mqtt_sub_topic_buf, 0, _az_COUNTOF(mqtt_sub_topic_buf));

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  assert_true(
      az_iot_hub_client_c2d_subscribe_topic_filter_get(&client, mqtt_sub_topic, &mqtt_sub_topic)
      == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(mqtt_sub_topic), 0);
  assert_int_equal(az_span_capacity(mqtt_sub_topic), _az_COUNTOF(g_test_correct_subscribe_topic) - 2);
}

static void test_az_iot_hub_client_c2d_received_topic_parse_URL_DECODED_succeed(void** state)
{
  (void)state;

  // az_pair pair;
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = test_URL_DECODED_topic;

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

static void test_az_iot_hub_client_c2d_received_topic_parse_URL_ENCODED_succeed(void** state)
{
  (void)state;

  // az_pair pair;
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = test_URL_ENCODED_topic;

  az_iot_hub_client_c2d_request out_request;

  assert_return_code(az_iot_hub_client_c2d_received_topic_parse(&client, received_topic, &out_request), AZ_OK);

  // TODO: enable after az_iot_hub_client_properties_next() is implemented.
  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("%24.to")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("%2Fdevices%2Fuseragent_c%2Fmessages%2FdeviceBound")));
  // 
  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("abc")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("123")));

  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("ghi")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("%2Fsome%2Fthing")));

  // assert_return_code(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  // assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("jkl")));
  // assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("%2Fsome%2Fthing%2F%3Fbla%3Dbla")));
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
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_no_properties_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_MALFORMED_fail),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_c2d_subscribe_topic_filter_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_subscribe_topic_filter_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_URL_DECODED_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_received_topic_parse_URL_ENCODED_succeed)
  };
  return cmocka_run_group_tests_name("az_iot_hub_c2d", tests, NULL, NULL);
}
