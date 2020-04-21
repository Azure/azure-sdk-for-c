// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_iot_provisioning_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

static const az_span test_global_device_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");

#define TEST_ID_SCOPE "0neFEEDC0DE"
static const az_span test_id_scope = AZ_SPAN_LITERAL_FROM_STR(TEST_ID_SCOPE);

#define TEST_REGISTRATION_ID "myRegistrationId"
static const az_span test_registration_id = AZ_SPAN_LITERAL_FROM_STR(TEST_REGISTRATION_ID);

#define TEST_USER_AGENT "c/1.2.3"
#define TEST_REQUEST_ID "9060edd6-cc37-43d7-a96e-1bb3c3abcb82"

static void test_az_iot_provisioning_client_options_default_succeed()
{
  az_iot_provisioning_client_options options = az_iot_provisioning_client_options_default();
  assert_true(az_span_is_content_equal(options.user_agent, AZ_SPAN_NULL));
}

static void test_az_iot_provisioning_client_default_options_get_connect_info_succeed()
{
  az_iot_provisioning_client client;
  az_result ret = az_iot_provisioning_client_init(
      &client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL);
  assert_int_equal(AZ_OK, ret);

  uint8_t client_id_buffer[128];
  az_span client_id = AZ_SPAN_FROM_BUFFER(client_id_buffer);
  az_span_fill(client_id, 0xCC);

  ret = az_iot_provisioning_client_id_get(&client, client_id, &client_id);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(
      az_span_ptr(test_registration_id), az_span_ptr(client_id), (size_t)az_span_size(client_id));
  assert_int_equal(0xCC, client_id_buffer[az_span_size(client_id)]);

  uint8_t username_buffer[128];
  az_span username = AZ_SPAN_FROM_BUFFER(username_buffer);
  az_span_fill(username, 0xCC);

  az_span expected_username
      = AZ_SPAN_LITERAL_FROM_STR(TEST_ID_SCOPE "/registrations/" TEST_REGISTRATION_ID
                                               "/api-version=" AZ_IOT_PROVISIONING_SERVICE_VERSION);

  ret = az_iot_provisioning_client_user_name_get(&client, username, &username);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(
      az_span_ptr(expected_username), az_span_ptr(username), (size_t)az_span_size(username));
  assert_int_equal(0xCC, username_buffer[az_span_size(username)]);
}

static void test_az_iot_provisioning_client_custom_options_get_username_succeed()
{
  az_iot_provisioning_client client;
  az_iot_provisioning_client_options options = az_iot_provisioning_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);

  az_result ret = az_iot_provisioning_client_init(
      &client, test_global_device_endpoint, test_id_scope, test_registration_id, &options);
  assert_int_equal(AZ_OK, ret);

  uint8_t username_buffer[128];
  az_span username = AZ_SPAN_FROM_BUFFER(username_buffer);
  az_span_fill(username, 0xCC);

  az_span expected_username = AZ_SPAN_LITERAL_FROM_STR(
      TEST_ID_SCOPE "/registrations/" TEST_REGISTRATION_ID
                    "/api-version=" AZ_IOT_PROVISIONING_SERVICE_VERSION "&" TEST_USER_AGENT);

  ret = az_iot_provisioning_client_user_name_get(&client, username, &username);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(
      az_span_ptr(expected_username), az_span_ptr(username), (size_t)az_span_size(username));
  assert_int_equal(0xCC, username_buffer[az_span_size(username)]);
}

static void test_az_iot_provisioning_client_get_subscribe_topic_filter_succeed()
{
  az_iot_provisioning_client client;
  uint8_t topic_buffer[128];
  az_span topic = AZ_SPAN_FROM_BUFFER(topic_buffer);
  az_span_fill(topic, 0xCC);

  az_span expected_topic = AZ_SPAN_LITERAL_FROM_STR("$dps/registrations/res/#");

  az_result ret
      = az_iot_provisioning_client_register_subscribe_topic_filter_get(&client, topic, &topic);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(az_span_ptr(expected_topic), az_span_ptr(topic), (size_t)az_span_size(topic));
  assert_int_equal(0xCC, topic_buffer[az_span_size(topic)]);
}

static void test_az_iot_provisioning_client_get_register_publish_topic_filter_succeed()
{
  az_iot_provisioning_client client;
  uint8_t topic_buffer[128];
  az_span topic = AZ_SPAN_FROM_BUFFER(topic_buffer);
  az_span_fill(topic, 0xCC);

  az_span expected_topic
      = AZ_SPAN_LITERAL_FROM_STR("$dps/registrations/PUT/iotdps-register/?$rid=1");

  az_result ret = az_iot_provisioning_client_register_publish_topic_get(&client, topic, &topic);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(az_span_ptr(expected_topic), az_span_ptr(topic), (size_t)az_span_size(topic));
  assert_int_equal(0xCC, topic_buffer[az_span_size(topic)]);
}

static void test_az_iot_provisioning_client_get_operation_status_publish_topic_filter_succeed()
{
  az_iot_provisioning_client client;
  uint8_t topic_buffer[128];
  az_span topic = AZ_SPAN_FROM_BUFFER(topic_buffer);
  az_span_fill(topic, 0xCC);

  az_span expected_topic
      = AZ_SPAN_LITERAL_FROM_STR("$dps/registrations/GET/iotdps-get-operationstatus/"
                                 "?$rid=1&operationId=4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-"
                                 "61d5e55dac0d" TEST_REQUEST_ID);

  az_span operation_id
      = AZ_SPAN_LITERAL_FROM_STR("4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d");
  az_iot_provisioning_client_register_response response = { 0 };
  response.operation_id = operation_id;

  az_result ret = az_iot_provisioning_client_get_operation_status_publish_topic_get(
      &client, &response, topic, &topic);

  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(az_span_ptr(expected_topic), az_span_ptr(topic), (size_t)az_span_size(topic));
  assert_int_equal(0xCC, topic_buffer[az_span_size(topic)]);
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_provisioning_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_provisioning_client_options_default_succeed),
    cmocka_unit_test(test_az_iot_provisioning_client_default_options_get_connect_info_succeed),
    cmocka_unit_test(test_az_iot_provisioning_client_custom_options_get_username_succeed),
    cmocka_unit_test(test_az_iot_provisioning_client_get_subscribe_topic_filter_succeed),
    cmocka_unit_test(test_az_iot_provisioning_client_get_register_publish_topic_filter_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_get_operation_status_publish_topic_filter_succeed),
  };

  return cmocka_run_group_tests_name("az_iot_provisioning_client", tests, NULL, NULL);
}
