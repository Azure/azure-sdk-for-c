// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_result.h>
#include <az_span.h>

#include <az_precondition.h>
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

static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static uint8_t g_expected_methods_subscribe_topic[] = "$iothub/methods/POST/#";

#ifndef NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void test_az_iot_hub_client_methods_get_subscribe_topic_filter_NULL_client_fail(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(
      az_iot_hub_client_methods_get_subscribe_topic_filter(NULL, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_methods_get_subscribe_topic_filter_NULL_out_topic_fail(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  assert_precondition_checked(az_iot_hub_client_methods_get_subscribe_topic_filter(
      &client, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_methods_get_subscribe_topic_filter_empty_topic_fail(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  assert_precondition_checked(az_iot_hub_client_methods_get_subscribe_topic_filter(
      &client, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_methods_get_response_publish_topic_NULL_client_fail(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;

  assert_precondition_checked(az_iot_hub_client_methods_get_response_publish_topic(
      NULL, request_id, status, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_methods_get_response_publish_topic_NULL_out_topic_fail(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;

  assert_precondition_checked(az_iot_hub_client_methods_get_response_publish_topic(
      &client, request_id, status, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_methods_get_response_publish_topic_zero_size_buffer_fail(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;

  assert_precondition_checked(az_iot_hub_client_methods_get_response_publish_topic(
      &client, request_id, status, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_methods_get_response_publish_topic_EMPTY_request_id_fail(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("");
  uint16_t status = 200;

  assert_precondition_checked(az_iot_hub_client_methods_get_response_publish_topic(
      &client, request_id, status, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_methods_get_response_publish_topic_AZ_SPAN_NULL_request_id_fail(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_NULL;
  uint16_t status = 200;

  assert_precondition_checked(az_iot_hub_client_methods_get_response_publish_topic(
      &client, request_id, status, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_methods_parse_received_topic_NULL_client_fail(void** state)
{
  (void)state;

  az_span received_topic = AZ_SPAN_FROM_STR("$iothub/methods/POST/TestMethod/?$rid=1");

  az_iot_hub_client_method_request out_request;

  assert_precondition_checked(
      az_iot_hub_client_methods_parse_received_topic(NULL, received_topic, &out_request));
}

static void test_az_iot_hub_client_methods_parse_received_topic_EMPTY_received_topic_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("");

  az_iot_hub_client_method_request out_request;

  assert_precondition_checked(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request));
}

static void test_az_iot_hub_client_methods_parse_received_topic_AZ_SPAN_NULL_received_topic_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = AZ_SPAN_NULL;

  az_iot_hub_client_method_request out_request;

  assert_precondition_checked(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request));
}

static void test_az_iot_hub_client_methods_parse_received_topic_NULL_out_request_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("$iothub/methods/POST/TestMethod/?$rid=1");

  assert_precondition_checked(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_methods_get_subscribe_topic_filter_succeed(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  assert_int_equal(
      az_iot_hub_client_methods_get_subscribe_topic_filter(
          &client, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);
  assert_string_equal(g_expected_methods_subscribe_topic, test_buf);
  assert_int_equal(sizeof(g_expected_methods_subscribe_topic) - 1, test_length);
}

static void test_az_iot_hub_client_methods_get_subscribe_topic_filter_INSUFFICIENT_BUFFER_fail(
    void** state)
{
  (void)state;

  char test_buf[5];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  assert_int_equal(
      az_iot_hub_client_methods_get_subscribe_topic_filter(
          &client, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_methods_get_response_publish_topic_succeed(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;
  const char expected_topic[] = "$iothub/methods/res/200/?$rid=2";

  assert_true(
      az_iot_hub_client_methods_get_response_publish_topic(
          &client, request_id, status, test_buf, sizeof(test_buf), &test_length)
      == AZ_OK);

  assert_string_equal(expected_topic, test_buf);
  assert_int_equal(sizeof(expected_topic) - 1, test_length);
}

static void
test_az_iot_hub_client_methods_get_response_publish_topic_INSUFFICIENT_BUFFER_for_prefix_fail(
    void** state)
{
  (void)state;

  char test_buf[10];
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;

  assert_true(
      az_iot_hub_client_methods_get_response_publish_topic(
          &client, request_id, status, test_buf, sizeof(test_buf), &test_length)
      == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void
test_az_iot_hub_client_methods_get_response_publish_topic_INSUFFICIENT_BUFFER_for_status_fail(
    void** state)
{
  (void)state;

  char test_buf[21];  // Enough for "$iothub/methods/res/2"
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;

  assert_true(
      az_iot_hub_client_methods_get_response_publish_topic(
          &client, request_id, status, test_buf, sizeof(test_buf), &test_length)
      == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void
test_az_iot_hub_client_methods_get_response_publish_topic_INSUFFICIENT_BUFFER_for_reqid_fail(
    void** state)
{
  (void)state;

  char test_buf[24]; // Enough for "$iothub/methods/res/200/"
  size_t test_length;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span request_id = AZ_SPAN_LITERAL_FROM_STR("2");
  uint16_t status = 200;

  assert_true(
      az_iot_hub_client_methods_get_response_publish_topic(
          &client, request_id, status, test_buf, sizeof(test_buf), &test_length)
      == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_methods_parse_received_topic_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  const char expected_name[] = "TestMethod";
  const char expected_request_id[] = "1";
  az_span received_topic = AZ_SPAN_FROM_STR("$iothub/methods/POST/TestMethod/?$rid=1");

  az_iot_hub_client_method_request out_request;

  assert_true(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request)
      == AZ_OK);
  assert_int_equal(az_span_size(out_request.name), _az_COUNTOF(expected_name) - 1);
  assert_memory_equal(az_span_ptr(out_request.name), expected_name, _az_COUNTOF(expected_name) - 1);
  assert_int_equal(az_span_size(out_request.request_id), _az_COUNTOF(expected_request_id) - 1);
  assert_memory_equal(
      az_span_ptr(out_request.request_id),
      expected_request_id,
      _az_COUNTOF(expected_request_id) - 1);
}

static void test_az_iot_hub_client_methods_parse_received_topic_c2d_topic_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic
      = AZ_SPAN_FROM_STR("$iothub/devices/useragent_c/messages/devicebound/"
                         "%24.to=%2Fdevices%2Fuseragent_c%2Fmessages%2FdeviceBound&abc=123&ghi=%"
                         "2Fsome%2Fthing&jkl=%2Fsome%2Fthing%2F%3Fbla%3Dbla");

  az_iot_hub_client_method_request out_request;

  assert_true(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request)
      == AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_methods_parse_received_topic_get_twin_topic_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("$iothub/twin/res/200/?$rid=2");

  az_iot_hub_client_method_request out_request;

  assert_true(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request)
      == AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_methods_parse_received_topic_twin_patch_topic_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("$iothub/twin/res/204/?$rid=4&$version=3");

  az_iot_hub_client_method_request out_request;

  assert_true(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request)
      == AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_methods_parse_received_topic_topic_filter_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = az_span_init(
      g_expected_methods_subscribe_topic, _az_COUNTOF(g_expected_methods_subscribe_topic));

  az_iot_hub_client_method_request out_request;

  assert_true(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request)
      == AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_methods_parse_received_topic_response_topic_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span received_topic = AZ_SPAN_FROM_STR("$iothub/methods/res/200/?$rid=2");

  az_iot_hub_client_method_request out_request;

  assert_true(
      az_iot_hub_client_methods_parse_received_topic(&client, received_topic, &out_request)
      == AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

int test_iot_hub_methods()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_methods_get_subscribe_topic_filter_NULL_client_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_get_subscribe_topic_filter_NULL_out_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_get_subscribe_topic_filter_empty_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_get_response_publish_topic_NULL_out_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_get_response_publish_topic_NULL_client_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_response_publish_topic_zero_size_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_response_publish_topic_EMPTY_request_id_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_response_publish_topic_AZ_SPAN_NULL_request_id_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_NULL_client_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_EMPTY_received_topic_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_parse_received_topic_AZ_SPAN_NULL_received_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_NULL_out_request_fail),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_methods_get_subscribe_topic_filter_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_subscribe_topic_filter_INSUFFICIENT_BUFFER_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_get_response_publish_topic_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_response_publish_topic_INSUFFICIENT_BUFFER_for_prefix_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_response_publish_topic_INSUFFICIENT_BUFFER_for_status_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_methods_get_response_publish_topic_INSUFFICIENT_BUFFER_for_reqid_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_succeed),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_c2d_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_get_twin_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_twin_patch_topic_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_topic_filter_fail),
    cmocka_unit_test(test_az_iot_hub_client_methods_parse_received_topic_response_topic_fail)
  };

  return cmocka_run_group_tests_name("az_iot_hub_methods", tests, NULL, NULL);
}
