// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_test_precondition.h>
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/internal/az_mqtt_policy_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

void az_platform_critical_error(void) { assert_true(false); }

// MQTT information for testing
#define TEST_MQTT_ENDPOINT "127.0.0.1"
#define TEST_MQTT_PORT 2883
#define TEST_MQTT_USERNAME ""
#define TEST_MQTT_PASSWORD ""
#define TEST_MQTT_CLIENT_ID "test_client_id"
#define TEST_MQTT_TOPIC "test_topic"
#define TEST_MQTT_PAYLOAD "test_payload"

#define TEST_MAX_RESPONSE_CHECKS 3
#define TEST_RESPONSE_DELAY_MS 100 // Response from endpoint is slow.

static az_mqtt test_mqtt_client;
static _az_mqtt_policy test_mqtt_policy;
static _az_hfsm test_inbound_hfsm;
static _az_event_pipeline test_event_pipeline;

// Set variables to track the number of times each event is called.
static int ref_connack = 0;
static int ref_suback = 0;
static int ref_puback = 0;
static int ref_recv = 0;
static int ref_disconnect = 0;

static az_mqtt_connect_data test_mqtt_connect_data = {
  .host = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_ENDPOINT),
  .port = TEST_MQTT_PORT,
  .use_username_password = false,
  .username = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_USERNAME),
  .password = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_PASSWORD),
  .client_id = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_CLIENT_ID),
  .certificate = { .cert = AZ_SPAN_LITERAL_EMPTY, .key = AZ_SPAN_LITERAL_EMPTY, .key_type = 0 },
};

static az_mqtt_options options = {
  .certificate_authority_trusted_roots = AZ_SPAN_LITERAL_EMPTY,
  .openssl_engine = AZ_SPAN_LITERAL_EMPTY,
  .mosquitto_handle = NULL,
  .use_tls = false,
};

static az_event_policy_handler az_inbound_hfsm_get_parent(az_event_policy_handler child_handler)
{
  (void)child_handler;
  return NULL;
}

static az_result test_inbound_hfsm_root(az_event_policy* me, az_event event)
{
  (void)me;
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_MQTT_EVENT_CONNECT_RSP:
      ref_connack++;
      break;

    case AZ_MQTT_EVENT_SUBACK_RSP:
      ref_suback++;
      break;

    case AZ_MQTT_EVENT_PUBACK_RSP:
      ref_puback++;
      break;

    case AZ_MQTT_EVENT_PUB_RECV_IND:
      ref_recv++;
      break;

    case AZ_MQTT_EVENT_DISCONNECT_RSP:
      ref_disconnect++;
      break;
  }

  return ret;
}

#ifndef AZ_NO_PRECONDITION_CHECKING

ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_mqtt_policy_init_null_failure(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_mqtt_init(NULL, NULL));
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_mqtt_policy_init_success(void** state)
{
  (void)state;

  ref_connack = 0;
  ref_suback = 0;
  ref_puback = 0;
  ref_recv = 0;
  ref_disconnect = 0;

  assert_int_equal(
      _az_hfsm_init(
          &test_inbound_hfsm,
          test_inbound_hfsm_root,
          az_inbound_hfsm_get_parent,
          (az_event_policy*)&test_mqtt_policy,
          NULL),
      AZ_OK);

  assert_int_equal(
      _az_mqtt_policy_init(
          &test_mqtt_policy, &test_mqtt_client, NULL, NULL, (az_event_policy*)&test_inbound_hfsm),
      AZ_OK);

  assert_int_equal(
      _az_event_pipeline_init(
          &test_event_pipeline,
          (az_event_policy*)&test_mqtt_policy,
          (az_event_policy*)&test_inbound_hfsm),
      AZ_OK);
}

static void test_az_mqtt_policy_init_valid_success(void** state)
{
  (void)state;

  assert_int_equal(az_mqtt_init(&test_mqtt_client, &options), AZ_OK);

  test_mqtt_client._internal.platform_mqtt.pipeline = &test_event_pipeline;
}

static void test_az_mqtt_policy_outbound_connect_success(void** state)
{
  (void)state;
  ref_connack = 0;

  assert_int_equal(az_mqtt_outbound_connect(&test_mqtt_client, &test_mqtt_connect_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_connack == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_connack, 1);
}

static void test_az_mqtt_policy_outbound_sub_success(void** state)
{
  (void)state;
  ref_suback = 0;

  az_mqtt_sub_data test_mqtt_sub_data = {
    .topic_filter = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_TOPIC),
    .qos = 0,
    .out_id = 0,
  };

  assert_int_equal(az_mqtt_outbound_sub(&test_mqtt_client, &test_mqtt_sub_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_suback == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_suback, 1);
}

static void test_az_mqtt_policy_outbound_pub_success(void** state)
{
  (void)state;
  ref_puback = 0;
  ref_recv = 0;

  az_mqtt_pub_data test_mqtt_pub_data = {
    .topic = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_TOPIC),
    .qos = 0,
    .out_id = 0,
    .payload = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_PAYLOAD),
  };

  assert_int_equal(az_mqtt_outbound_pub(&test_mqtt_client, &test_mqtt_pub_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_puback == 0 && ref_recv == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_puback, 1);
  assert_true(ref_recv > 0);
}

static void test_az_mqtt_policy_outbound_disconnect_success(void** state)
{
  (void)state;
  ref_disconnect = 0;

  assert_int_equal(az_mqtt_outbound_disconnect(&test_mqtt_client), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_disconnect == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_disconnect, 1);
}

int test_az_mqtt_policy()
{
  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_mqtt_policy_init_null_failure),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_mqtt_policy_init_success),
    cmocka_unit_test(test_az_mqtt_policy_init_valid_success),
    cmocka_unit_test(test_az_mqtt_policy_outbound_connect_success),
    cmocka_unit_test(test_az_mqtt_policy_outbound_sub_success),
    cmocka_unit_test(test_az_mqtt_policy_outbound_pub_success),
    cmocka_unit_test(test_az_mqtt_policy_outbound_disconnect_success),
  };
  return cmocka_run_group_tests_name("az_mqtt_policy", tests, NULL, NULL);
}
