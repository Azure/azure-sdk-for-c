// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_span.h>
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

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR("my_device");
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR("myiothub.azure-devices.net");
static const az_span test_device_request_id = AZ_SPAN_LITERAL_FROM_STR("id_one");
static const az_span test_twin_received_topic_desired_success
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/PATCH/properties/desired/?$version=id_one");
static const az_span test_twin_received_topic_fail
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/rez/200");
static const az_span test_twin_received_topic_prefix_fail
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/contoso/res/200");

static const az_span test_twin_received_get_response
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/res/200/?$rid=id_one");
static const az_span test_twin_reported_props_success_response
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/res/204/?$rid=id_one&$version=16");

static const char test_correct_twin_response_topic_filter[] = "$iothub/twin/res/#";
static const char test_correct_twin_get_request_topic[] = "$iothub/twin/GET/?$rid=id_one";
static const char test_correct_twin_path_subscribe_topic[]
    = "$iothub/twin/PATCH/properties/desired/#";
static const char test_correct_twin_patch_pub_topic[]
    = "$iothub/twin/PATCH/properties/reported/?$rid=id_one";

#ifndef NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

    static void test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_null_client_fails(
        void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_response_get_subscribe_topic_filter(
      NULL, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_NULL_mqtt_buf_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_response_get_subscribe_topic_filter(
      &client, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_NULL_out_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_response_get_subscribe_topic_filter(
      &client, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_NULL_client_fails(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_patch_get_subscribe_topic_filter(
      NULL, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_NULL_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_patch_get_subscribe_topic_filter(
      &client, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_NULL_out_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(
      az_iot_hub_client_twin_patch_get_subscribe_topic_filter(&client, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_twin_document_get_publish_topic_NULL_client_fails(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_document_get_publish_topic(
      NULL, test_device_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_document_get_publish_topic_NULL_request_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_init(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_twin_document_get_publish_topic(
      &client, test_bad_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_document_get_publish_topic_NULL_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_document_get_publish_topic(
      &client, test_device_request_id, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_document_get_publish_topic_NULL_out_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_document_get_publish_topic(
      &client, test_device_request_id, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_publish_topic_NULL_client_fails(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_patch_get_publish_topic(
      NULL, test_device_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_publish_topic_invalid_request_id_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_init(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_twin_patch_get_publish_topic(
      &client, test_bad_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_publish_topic_NULL_char_buf_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_patch_get_publish_topic(
      &client, test_device_request_id, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_twin_patch_get_publish_topic_NULL_out_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_precondition_checked(az_iot_hub_client_twin_patch_get_publish_topic(
      &client, test_device_request_id, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_twin_parse_received_topic_NULL_client_fails(void** state)
{
  (void)state;

  az_iot_hub_client_twin_response response;

  assert_precondition_checked(az_iot_hub_client_twin_parse_received_topic(
      NULL, test_twin_received_topic_desired_success, &response));
}

static void test_az_iot_hub_client_twin_parse_received_topic_NULL_rec_topic_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_twin_response response;

  assert_precondition_checked(
      az_iot_hub_client_twin_parse_received_topic(&client, AZ_SPAN_NULL, &response));
}

static void test_az_iot_hub_client_twin_parse_received_topic_NULL_response_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);
  ;

  assert_precondition_checked(az_iot_hub_client_twin_parse_received_topic(
      &client, test_twin_received_topic_desired_success, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_response_get_subscribe_topic_filter(
          &client, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_correct_twin_response_topic_filter, test_buf);
  assert_int_equal(sizeof(test_correct_twin_response_topic_filter) - 1, test_length);
}

static void test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[sizeof(test_correct_twin_response_topic_filter) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_response_get_subscribe_topic_filter(
          &client, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_twin_document_get_publish_topic_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_document_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_twin_get_request_topic, test_buf);
  assert_int_equal(sizeof(test_correct_twin_get_request_topic) - 1, test_length);
}

static void test_az_iot_hub_client_twin_document_get_publish_topic_small_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[_az_COUNTOF(test_correct_twin_get_request_topic) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_document_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_patch_get_subscribe_topic_filter(
          &client, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_twin_path_subscribe_topic, test_buf);
  assert_int_equal(sizeof(test_correct_twin_path_subscribe_topic) - 1, test_length);
}

static void test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[_az_COUNTOF(test_correct_twin_path_subscribe_topic) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_patch_get_subscribe_topic_filter(
          &client, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_twin_patch_get_publish_topic_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_patch_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_twin_patch_pub_topic, test_buf);
  assert_int_equal(sizeof(test_correct_twin_patch_pub_topic) - 1, test_length);
}

static void test_az_iot_hub_client_twin_patch_get_publish_topic_small_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[_az_COUNTOF(test_correct_twin_patch_pub_topic) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_twin_patch_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_twin_parse_received_topic_desired_found_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);
  az_iot_hub_client_twin_response response;

  assert_int_equal(
      az_iot_hub_client_twin_parse_received_topic(
          &client, test_twin_received_topic_desired_success, &response),
      AZ_OK);
  assert_true(az_span_is_content_equal((response.version), test_device_request_id));
  assert_true(az_span_is_content_equal(response.request_id, AZ_SPAN_NULL));
  assert_int_equal(response.status, AZ_IOT_STATUS_OK);
  assert_int_equal(response.response_type, AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES);
}

static void test_az_iot_hub_client_twin_parse_received_topic_get_response_found_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);
  az_iot_hub_client_twin_response response;

  assert_int_equal(
      az_iot_hub_client_twin_parse_received_topic(
          &client, test_twin_received_get_response, &response),
      AZ_OK);
  assert_true(az_span_is_content_equal(response.request_id, test_device_request_id));
  assert_true(az_span_is_content_equal(response.version, AZ_SPAN_NULL));
  assert_int_equal(response.status, AZ_IOT_STATUS_OK);
  assert_int_equal(response.response_type, AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET);
}

static void test_az_iot_hub_client_twin_parse_received_topic_reported_props_found_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);
  az_iot_hub_client_twin_response response;

  assert_int_equal(
      az_iot_hub_client_twin_parse_received_topic(
          &client, test_twin_reported_props_success_response, &response),
      AZ_OK);
  assert_true(az_span_is_content_equal(response.request_id, test_device_request_id));
  assert_true(az_span_is_content_equal(response.version, AZ_SPAN_FROM_STR("16")));
  assert_int_equal(response.status, AZ_IOT_STATUS_NO_CONTENT);
  assert_int_equal(response.response_type, AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES);
}

static void test_az_iot_hub_client_twin_parse_received_topic_not_found_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);
  az_iot_hub_client_twin_response response;

  assert_int_equal(
      az_iot_hub_client_twin_parse_received_topic(
          &client, test_twin_received_topic_fail, &response),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_twin_parse_received_topic_not_found_prefix_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);
  az_iot_hub_client_twin_response response;

  assert_int_equal(
      az_iot_hub_client_twin_parse_received_topic(
          &client, test_twin_received_topic_prefix_fail, &response),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

int test_az_iot_hub_client_twin()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_null_client_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_NULL_mqtt_buf_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_NULL_out_span_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_NULL_span_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_document_get_publish_topic_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_document_get_publish_topic_NULL_request_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_document_get_publish_topic_NULL_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_document_get_publish_topic_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_publish_topic_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_publish_topic_invalid_request_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_publish_topic_NULL_char_buf_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_publish_topic_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_NULL_rec_topic_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_NULL_response_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_get_subscribe_topic_filter_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_document_get_publish_topic_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_document_get_publish_topic_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_patch_get_subscribe_topic_filter_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_publish_topic_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_get_publish_topic_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_desired_found_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_get_response_found_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_reported_props_found_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_not_found_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_parse_received_topic_not_found_prefix_fails),
  };

  return cmocka_run_group_tests_name("az_iot_hub_client_twin", tests, NULL, NULL);
}
