// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_iot_provisioning_client.h>
#include <az_span.h>
#include <az_test_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
#include <_az_cfg.h>

static const az_span test_global_device_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");

#define TEST_ID_SCOPE "0neFEEDC0DE"
#define TEST_REGISTRATION_ID "myRegistrationId"
#define TEST_USER_AGENT "c/1.2.3"
#define TEST_OPERATION_ID "4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d"

#define USER_AGENT_PREFIX "&ClientVersion="

static void test_az_iot_provisioning_client_options_default_succeed()
{
  az_iot_provisioning_client_options options = az_iot_provisioning_client_options_default();
  assert_true(az_span_is_content_equal(options.user_agent, AZ_SPAN_NULL));
}

static void test_az_iot_provisioning_client_default_options_get_connect_info_succeed()
{
  az_iot_provisioning_client client;
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_endpoint,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  char client_id[128];
  memset(client_id, 0xCC, sizeof(client_id));
  ret = az_iot_provisioning_client_get_client_id(&client, client_id, sizeof(client_id), NULL);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(TEST_REGISTRATION_ID, client_id, strlen(client_id));
  assert_int_equal((uint8_t)0xCC, (uint8_t)client_id[strlen(client_id) + 1]);

  char user_name[128];
  memset(user_name, 0xCC, sizeof(user_name));
  char* expected_username = TEST_ID_SCOPE "/registrations/" TEST_REGISTRATION_ID
                                          "/api-version=" AZ_IOT_PROVISIONING_SERVICE_VERSION;

  ret = az_iot_provisioning_client_get_user_name(&client, user_name, sizeof(user_name), NULL);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(expected_username, user_name, strlen(user_name));
  assert_int_equal((uint8_t)0xCC, (uint8_t)user_name[strlen(user_name) + 1]);
}

static void test_az_iot_provisioning_client_custom_options_get_username_succeed()
{
  az_iot_provisioning_client client;
  az_iot_provisioning_client_options options = az_iot_provisioning_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);

  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_endpoint,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      &options);
  assert_int_equal(AZ_OK, ret);

  char user_name[128];
  memset(user_name, 0xCC, sizeof(user_name));

  char* expected_username = TEST_ID_SCOPE
      "/registrations/" TEST_REGISTRATION_ID
      "/api-version=" AZ_IOT_PROVISIONING_SERVICE_VERSION USER_AGENT_PREFIX TEST_USER_AGENT;

  ret = az_iot_provisioning_client_get_user_name(&client, user_name, sizeof(user_name), NULL);
  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(expected_username, user_name, strlen(user_name));
  assert_int_equal((uint8_t)0xCC, (uint8_t)user_name[strlen(user_name) + 1]);
}

static void test_az_iot_provisioning_client_get_subscribe_topic_filter_succeed()
{
  az_iot_provisioning_client client;
  char topic[128];
  memset(topic, 0xCC, sizeof(topic));

  char* expected_topic = "$dps/registrations/res/#";

  az_result ret = az_iot_provisioning_client_register_get_subscribe_topic_filter(
      &client, topic, sizeof(topic), NULL);

  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(expected_topic, topic, strlen(topic));
  assert_int_equal((uint8_t)0xCC, (uint8_t)topic[strlen(topic) + 1]);
}

static void test_az_iot_provisioning_client_get_register_publish_topic_filter_succeed()
{
  az_iot_provisioning_client client;
  char topic[128];
  memset(topic, 0xCC, sizeof(topic));

  char* expected_topic = "$dps/registrations/PUT/iotdps-register/?$rid=1";

  az_result ret
      = az_iot_provisioning_client_register_get_publish_topic(&client, topic, sizeof(topic), NULL);

  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(expected_topic, topic, strlen(topic));
  assert_int_equal((uint8_t)0xCC, (uint8_t)topic[strlen(topic) + 1]);
}

static void test_az_iot_provisioning_client_get_operation_status_publish_topic_filter_succeed()
{
  az_iot_provisioning_client client;
  char topic[128];
  memset(topic, 0xCC, sizeof(topic));

  char* expected_topic
      = "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=1&operationId=" TEST_OPERATION_ID;

  az_span operation_id = AZ_SPAN_LITERAL_FROM_STR(TEST_OPERATION_ID);
  az_iot_provisioning_client_register_response response = { 0 };
  response.operation_id = operation_id;

  az_result ret = az_iot_provisioning_client_query_status_get_publish_topic(
      &client, &response, topic, sizeof(topic), NULL);

  assert_int_equal(AZ_OK, ret);
  assert_memory_equal(expected_topic, topic, strlen(topic));
  assert_int_equal((uint8_t)0xCC, (uint8_t)topic[strlen(topic) + 1]);
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
