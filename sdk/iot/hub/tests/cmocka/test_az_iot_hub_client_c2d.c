// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_log.h>
#include <az_precondition.h>
#include <az_span.h>

#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <az_test_span.h>
#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 128

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"

static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static char g_test_correct_subscribe_topic[] = "devices/my_device/messages/devicebound/#";
static const az_span test_url_no_props
    = AZ_SPAN_LITERAL_FROM_STR("devices/useragent_c/messages/devicebound/");
static const az_span test_url_decoded_topic = AZ_SPAN_LITERAL_FROM_STR(
    "devices/useragent_c/messages/devicebound/$.mid=79eadb01-bd0d-472d-bd35-ccb76e70eab8&$.to=/"
    "devices/useragent_c/messages/deviceBound&abc=123");
static const az_span test_url_encoded_topic
    = AZ_SPAN_LITERAL_FROM_STR("devices/useragent_c/messages/devicebound/"
                               "%24.to=%2Fdevices%2Fuseragent_c%2Fmessages%2FdeviceBound&abc=123&"
                               "ghi=%2Fsome%2Fthing&jkl=%2Fsome%2Fthing%2F%3Fbla%3Dbla");
static const az_span test_parse_method_topic_fail
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/methods/POST/foo/?$rid=one");

#ifndef AZ_NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void test_az_iot_hub_client_c2d_parse_received_topic_NULL_client_fail()
{
  az_span received_topic = AZ_SPAN_FROM_STR(
      "devices/useragent_c/messages/devicebound/$.mid=79eadb01-bd0d-472d-bd35-ccb76e70eab8&$.to=/"
      "devices/useragent_c/messages/deviceBound&iothub-ack=full");

  az_iot_hub_client_c2d_request out_request;

  assert_precondition_checked(
      az_iot_hub_client_c2d_parse_received_topic(NULL, received_topic, &out_request));
}

static void test_az_iot_hub_client_c2d_parse_received_topic_AZ_SPAN_NULL_received_topic_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_NULL;

  az_iot_hub_client_c2d_request out_request;

  assert_precondition_checked(
      az_iot_hub_client_c2d_parse_received_topic(&client, received_topic, &out_request));
}

static void test_az_iot_hub_client_c2d_parse_received_topic_NULL_out_request_fail()
{
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = test_url_decoded_topic;

  assert_precondition_checked(
      az_iot_hub_client_c2d_parse_received_topic(&client, received_topic, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_c2d_get_subscribe_topic_filter_succeed()
{
  char mqtt_sub_topic_buf[TEST_SPAN_BUFFER_SIZE];

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  size_t returned_size;
  assert_true(
      az_iot_hub_client_c2d_get_subscribe_topic_filter(
          &client, mqtt_sub_topic_buf, sizeof(mqtt_sub_topic_buf), &returned_size)
      == AZ_OK);
  assert_string_equal(mqtt_sub_topic_buf, g_test_correct_subscribe_topic);
  assert_int_equal(returned_size, sizeof(g_test_correct_subscribe_topic) - 1);
}

static void test_az_iot_hub_client_c2d_get_subscribe_topic_filter_small_buffer_fail()
{
  char mqtt_sub_topic_buf[_az_COUNTOF(g_test_correct_subscribe_topic) - 2];

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  size_t returned_size;
  assert_true(
      az_iot_hub_client_c2d_get_subscribe_topic_filter(
          &client, mqtt_sub_topic_buf, sizeof(mqtt_sub_topic_buf), &returned_size)
      == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_c2d_parse_received_topic_url_decoded_succeed()
{
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = test_url_decoded_topic;

  az_iot_hub_client_c2d_request out_request;

  assert_int_equal(
      az_iot_hub_client_c2d_parse_received_topic(&client, received_topic, &out_request), AZ_OK);

  az_pair pair;
  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("$.mid")));
  assert_true(az_span_is_content_equal(
      pair.value, AZ_SPAN_FROM_STR("79eadb01-bd0d-472d-bd35-ccb76e70eab8")));

  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("$.to")));
  assert_true(az_span_is_content_equal(
      pair.value, AZ_SPAN_FROM_STR("/devices/useragent_c/messages/deviceBound")));

  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("abc")));
  assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("123")));
}

static void test_az_iot_hub_client_c2d_parse_received_topic_url_encoded_succeed()
{
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = test_url_encoded_topic;

  az_iot_hub_client_c2d_request out_request;

  assert_int_equal(
      az_iot_hub_client_c2d_parse_received_topic(&client, received_topic, &out_request), AZ_OK);

  az_pair pair;
  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("%24.to")));
  assert_true(az_span_is_content_equal(
      pair.value, AZ_SPAN_FROM_STR("%2Fdevices%2Fuseragent_c%2Fmessages%2FdeviceBound")));

  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("abc")));
  assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("123")));

  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("ghi")));
  assert_true(az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("%2Fsome%2Fthing")));

  assert_int_equal(az_iot_hub_client_properties_next(&out_request.properties, &pair), AZ_OK);
  assert_true(az_span_is_content_equal(pair.key, AZ_SPAN_FROM_STR("jkl")));
  assert_true(
      az_span_is_content_equal(pair.value, AZ_SPAN_FROM_STR("%2Fsome%2Fthing%2F%3Fbla%3Dbla")));
}

static void test_az_iot_hub_client_c2d_parse_received_topic_no_props_succeed()
{
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_iot_hub_client_c2d_request out_request;

  assert_int_equal(
      az_iot_hub_client_c2d_parse_received_topic(&client, test_url_no_props, &out_request), AZ_OK);
}

static void test_az_iot_hub_client_c2d_parse_received_topic_reject()
{
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = test_parse_method_topic_fail;

  az_iot_hub_client_c2d_request out_request;

  assert_int_equal(
      az_iot_hub_client_c2d_parse_received_topic(&client, received_topic, &out_request),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_c2d_parse_received_topic_malformed_reject()
{
  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("devices/useragent_c/message#$vicebound/a=1");

  az_iot_hub_client_c2d_request out_request;

  assert_int_equal(
      az_iot_hub_client_c2d_parse_received_topic(&client, received_topic, &out_request),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static int _log_invoked_topic = 0;
static void _log_listener(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
      assert_memory_equal(
          az_span_ptr(test_url_no_props), az_span_ptr(message), (size_t)az_span_size(message));
      _log_invoked_topic++;
      break;
    default:
      assert_true(false);
  }
}

static void test_az_iot_hub_client_c2d_logging_succeed()
{
  az_log_classification const classifications[]
      = { AZ_LOG_MQTT_RECEIVED_TOPIC, AZ_LOG_MQTT_RECEIVED_PAYLOAD, AZ_LOG_END_OF_LIST };
  az_log_set_callback(_log_listener);
  az_log_set_classifications(classifications);

  assert_int_equal(0, _log_invoked_topic);

  _log_invoked_topic = 0;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_iot_hub_client_c2d_request out_request;
  assert_int_equal(
      az_iot_hub_client_c2d_parse_received_topic(&client, test_url_no_props, &out_request), AZ_OK);

  assert_int_equal(1, _log_invoked_topic);

  az_log_set_classifications(NULL);
  az_log_set_callback(NULL);
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_iot_hub_c2d()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_NULL_client_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_c2d_parse_received_topic_AZ_SPAN_NULL_received_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_NULL_out_request_fail),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_c2d_get_subscribe_topic_filter_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_get_subscribe_topic_filter_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_url_decoded_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_url_encoded_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_no_props_succeed),
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_reject),
    cmocka_unit_test(test_az_iot_hub_client_c2d_parse_received_topic_malformed_reject),
    cmocka_unit_test(test_az_iot_hub_client_c2d_logging_succeed),
  };
  return cmocka_run_group_tests_name("az_iot_hub_c2d", tests, NULL, NULL);
}
