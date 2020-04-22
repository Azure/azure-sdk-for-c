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

static const char test_correct_twin_response_topic_filter[] = "$iothub/twin/res/#";
static const char test_correct_twin_get_request_topic[] = "$iothub/twin/GET/?$rid=id_one";
static const char test_correct_twin_path_subscribe_topic[]
    = "$iothub/twin/PATCH/properties/desired/#";
static const char test_correct_twin_patch_pub_topic[]
    = "$iothub/twin/PATCH/properties/reported/?$rid=id_one";

#ifndef NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_null_client_fails(
    void** state)
{
  (void)state;

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(NULL, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_NULL_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));
  test_span._internal.ptr = NULL;

  assert_precondition_checked(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(&client, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_NULL_out_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(&client, test_span, NULL));
}

static void test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_NULL_client_fails(
    void** state)
{
  (void)state;

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(NULL, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_NULL_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));
  test_span._internal.ptr = NULL;

  assert_precondition_checked(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(&client, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_NULL_out_span_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(&client, test_span, NULL));
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_twin_get_publish_topic_get(
      NULL, test_device_request_id, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_NULL_request_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_get_request_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_init(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_twin_get_publish_topic_get(
      &client, test_bad_request_id, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_NULL_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_get_request_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));
  test_span._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_twin_get_publish_topic_get(
      &client, test_device_request_id, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_NULL_out_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_get_request_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_twin_get_publish_topic_get(
      &client, test_device_request_id, test_span, NULL));
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_patch_pub_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_twin_patch_publish_topic_get(
      NULL, test_device_request_id, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_invalid_request_id_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_patch_pub_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_init(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_twin_patch_publish_topic_get(
      &client, test_bad_request_id, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_NULL_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_patch_pub_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));
  test_span._internal.ptr = NULL;

  assert_precondition_checked(az_iot_hub_client_twin_patch_publish_topic_get(
      &client, test_device_request_id, test_span, &test_span));
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_NULL_out_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_patch_pub_topic) - 1];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_twin_patch_publish_topic_get(
      &client, test_device_request_id, test_span, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_twin_response_topic_filter,
      _az_COUNTOF(test_correct_twin_response_topic_filter) - 1,
      az_span_init(test_span_buf, _az_COUNTOF(test_span_buf)),
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_twice_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  memset(test_span_buf, 0xFF, _az_COUNTOF(test_span_buf));
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_response_topic_filter,
      _az_COUNTOF(test_correct_twin_response_topic_filter) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_response_topic_filter) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);

  assert_int_equal(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_response_topic_filter,
      _az_COUNTOF(test_correct_twin_response_topic_filter) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_response_topic_filter) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);
}

static void test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_response_topic_filter) - 2];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_response_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
  assert_int_equal(
      az_span_size(test_span), _az_COUNTOF(test_correct_twin_response_topic_filter) - 2);
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_get_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_twin_get_request_topic,
      _az_COUNTOF(test_correct_twin_get_request_topic) - 1,
      az_span_init(test_span_buf, _az_COUNTOF(test_span_buf)),
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_twice_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  memset(test_span_buf, 0xFF, _az_COUNTOF(test_span_buf));
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_get_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_get_request_topic,
      _az_COUNTOF(test_correct_twin_get_request_topic) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_get_request_topic) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);

  assert_int_equal(
      az_iot_hub_client_twin_get_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_get_request_topic,
      _az_COUNTOF(test_correct_twin_get_request_topic) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_get_request_topic) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);
}

static void test_az_iot_hub_client_twin_get_publish_topic_get_small_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_get_request_topic) - 2];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_get_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
  assert_int_equal(az_span_size(test_span), _az_COUNTOF(test_correct_twin_get_request_topic) - 2);
}

static void test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_twin_path_subscribe_topic,
      _az_COUNTOF(test_correct_twin_path_subscribe_topic) - 1,
      az_span_init(test_span_buf, _az_COUNTOF(test_span_buf)),
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_twice_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  memset(test_span_buf, 0xFF, _az_COUNTOF(test_span_buf));
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_path_subscribe_topic,
      _az_COUNTOF(test_correct_twin_path_subscribe_topic) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_path_subscribe_topic) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);

  assert_int_equal(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_path_subscribe_topic,
      _az_COUNTOF(test_correct_twin_path_subscribe_topic) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_path_subscribe_topic) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);
}

static void test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_path_subscribe_topic) - 2];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_patch_subscribe_topic_filter_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
  assert_int_equal(
      az_span_size(test_span), _az_COUNTOF(test_correct_twin_path_subscribe_topic) - 2);
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_patch_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_twin_patch_pub_topic,
      _az_COUNTOF(test_correct_twin_patch_pub_topic) - 1,
      az_span_init(test_span_buf, _az_COUNTOF(test_span_buf)),
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_twice_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  memset(test_span_buf, 0xFF, _az_COUNTOF(test_span_buf));
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_patch_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_patch_pub_topic,
      _az_COUNTOF(test_correct_twin_patch_pub_topic) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_patch_pub_topic) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);

  assert_int_equal(
      az_iot_hub_client_twin_patch_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_OK);
  assert_memory_equal(
      az_span_ptr(test_span),
      test_correct_twin_patch_pub_topic,
      _az_COUNTOF(test_correct_twin_patch_pub_topic) - 1);
  assert_int_equal(az_span_size(test_span), sizeof(test_correct_twin_patch_pub_topic) - 1);
  assert_int_equal(test_span_buf[az_span_size(test_span)], 0xFF);
}

static void test_az_iot_hub_client_twin_patch_publish_topic_get_small_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[_az_COUNTOF(test_correct_twin_patch_pub_topic) - 2];
  az_span test_span = az_span_init(test_span_buf, _az_COUNTOF(test_span_buf));

  assert_int_equal(
      az_iot_hub_client_twin_patch_publish_topic_get(
          &client, test_device_request_id, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
  assert_int_equal(az_span_size(test_span), _az_COUNTOF(test_correct_twin_patch_pub_topic) - 2);
}

int test_az_iot_hub_client_twin()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_null_client_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_NULL_span_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_NULL_out_span_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_NULL_span_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_NULL_request_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_NULL_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_NULL_out_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_invalid_request_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_NULL_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_NULL_out_span_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_twice_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_response_subscribe_topic_filter_get_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_get_publish_topic_get_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_twice_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_twin_patch_subscribe_topic_filter_get_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_twin_patch_publish_topic_get_small_buffer_fails),
  };

  return cmocka_run_group_tests_name("az_iot_hub_client_twin", tests, NULL, NULL);
}
