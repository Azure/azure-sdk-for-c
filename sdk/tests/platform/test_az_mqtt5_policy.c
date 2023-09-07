// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_test_precondition.h>
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/internal/az_mqtt5_policy_internal.h>
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
#define TEST_MQTT_PROPERTY_CONTENT_TYPE "test/content_type"
#define TEST_MQTT_PROPERTY_STRING_PAIR_KEY1 "test_key1"
#define TEST_MQTT_PROPERTY_STRING_PAIR_VALUE1 "test_value1"
#define TEST_MQTT_PROPERTY_STRING_PAIR_KEY2 "test_key2"
#define TEST_MQTT_PROPERTY_STRING_PAIR_VALUE2 "test_value2"
#define TEST_MQTT_PROPERTY_MESSAGE_EXPIRY_INTERVAL 1000
#define TEST_MQTT_PROPERTY_PAYLOAD_FORMAT_INDICATOR 0
#define TEST_MQTT_PROPERTY_CORRELATION_DATA "1234"

#define TEST_MAX_RESPONSE_CHECKS 3
#define TEST_RESPONSE_DELAY_MS 100 // Response from endpoint is slow.

#ifdef TRANSPORT_MOSQUITTO
static struct mosquitto* test_impl_handle = NULL;
#elif TRANSPORT_PAHO
static MQTTAsync test_impl_handle;
#else // TRANSPORT_NONE
static void* test_impl_handle = NULL;
#endif

static az_mqtt5 test_mqtt5_client;
static _az_mqtt5_policy test_mqtt5_policy;
static _az_hfsm test_inbound_hfsm;
static _az_event_pipeline test_event_pipeline;

// Set variables to track the number of times each event is called.
static int ref_connack = 0;
static int ref_suback = 0;
static int ref_puback = 0;
static int ref_recv = 0;
static int ref_disconnect = 0;

static int expect_msg_properties = 0;

static az_mqtt5_connect_data test_mqtt5_connect_data = {
  .host = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_ENDPOINT),
  .port = TEST_MQTT_PORT,
  .username = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_USERNAME),
  .password = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_PASSWORD),
  .client_id = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_CLIENT_ID),
  .certificate = { .cert = AZ_SPAN_LITERAL_EMPTY, .key = AZ_SPAN_LITERAL_EMPTY },
  .properties = NULL,
};

static az_mqtt5_options options = {
  .certificate_authority_trusted_roots = AZ_SPAN_LITERAL_EMPTY,
  .disable_tls = true,
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
    case AZ_MQTT5_EVENT_CONNECT_RSP:
      ref_connack++;
      az_mqtt5_connack_data* test_connack_data = (az_mqtt5_connack_data*)event.data;
      assert_int_equal(test_connack_data->connack_reason, 0);
      break;

    case AZ_MQTT5_EVENT_SUBACK_RSP:
      ref_suback++;
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
      ref_puback++;
      break;

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      ref_recv++;

      az_mqtt5_recv_data* test_recv_data = (az_mqtt5_recv_data*)event.data;
      assert_true(
          az_span_is_content_equal(test_recv_data->topic, AZ_SPAN_FROM_STR(TEST_MQTT_TOPIC)));
      assert_true(
          az_span_is_content_equal(test_recv_data->payload, AZ_SPAN_FROM_STR(TEST_MQTT_PAYLOAD)));

      if (expect_msg_properties != 0)
      {
        az_mqtt5_property_string test_mqtt5_property_string = AZ_MQTT5_PROPERTY_STRING_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_read_string(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
                &test_mqtt5_property_string),
            AZ_OK);
        assert_true(az_span_is_content_equal(
            az_mqtt5_property_get_string(&test_mqtt5_property_string),
            AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_CONTENT_TYPE)));
        az_mqtt5_property_free_string(&test_mqtt5_property_string);

        az_mqtt5_property_stringpair test_mqtt5_property_string_pair1
            = AZ_MQTT5_PROPERTY_STRINGPAIR_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_find_stringpair(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
                AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY1),
                &test_mqtt5_property_string_pair1),
            AZ_OK);
        assert_true(az_span_is_content_equal(
            az_mqtt5_property_stringpair_get_key(&test_mqtt5_property_string_pair1),
            AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY1)));
        assert_true(az_span_is_content_equal(
            az_mqtt5_property_stringpair_get_value(&test_mqtt5_property_string_pair1),
            AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_VALUE1)));
        az_mqtt5_property_free_stringpair(&test_mqtt5_property_string_pair1);

        az_mqtt5_property_stringpair test_mqtt5_property_string_pair2
            = AZ_MQTT5_PROPERTY_STRINGPAIR_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_find_stringpair(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
                AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY2),
                &test_mqtt5_property_string_pair2),
            AZ_OK);
        assert_true(az_span_is_content_equal(
            az_mqtt5_property_stringpair_get_key(&test_mqtt5_property_string_pair2),
            AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY2)));
        assert_true(az_span_is_content_equal(
            az_mqtt5_property_stringpair_get_value(&test_mqtt5_property_string_pair2),
            AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_VALUE2)));
        az_mqtt5_property_free_stringpair(&test_mqtt5_property_string_pair2);

        uint32_t test_mqtt5_property_int_32;
        assert_int_equal(
            az_mqtt5_property_bag_read_int(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_MESSAGE_EXPIRY_INTERVAL,
                &test_mqtt5_property_int_32),
            AZ_OK);
        assert_int_equal(test_mqtt5_property_int_32, TEST_MQTT_PROPERTY_MESSAGE_EXPIRY_INTERVAL);

        uint8_t test_mqtt5_property_int_8;
        assert_int_equal(
            az_mqtt5_property_bag_read_byte(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_PAYLOAD_FORMAT_INDICATOR,
                &test_mqtt5_property_int_8),
            AZ_OK);
        assert_int_equal(test_mqtt5_property_int_8, TEST_MQTT_PROPERTY_PAYLOAD_FORMAT_INDICATOR);

        az_mqtt5_property_binarydata test_mqtt5_property_binary_data
            = AZ_MQTT5_PROPERTY_BINARYDATA_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_read_binarydata(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
                &test_mqtt5_property_binary_data),
            AZ_OK);
        assert_true(az_span_is_content_equal(
            test_mqtt5_property_binary_data.bindata,
            AZ_SPAN_FROM_BUFFER(TEST_MQTT_PROPERTY_CORRELATION_DATA)));
        az_mqtt5_property_free_binarydata(&test_mqtt5_property_binary_data);
      }
      else
      {
        az_mqtt5_property_string test_mqtt5_property_string = AZ_MQTT5_PROPERTY_STRING_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_read_string(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
                &test_mqtt5_property_string),
            AZ_ERROR_ITEM_NOT_FOUND);

        az_mqtt5_property_stringpair test_mqtt5_property_string_pair1
            = AZ_MQTT5_PROPERTY_STRINGPAIR_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_find_stringpair(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
                AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY1),
                &test_mqtt5_property_string_pair1),
            AZ_ERROR_ITEM_NOT_FOUND);

        az_mqtt5_property_stringpair test_mqtt5_property_string_pair2
            = AZ_MQTT5_PROPERTY_STRINGPAIR_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_find_stringpair(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
                AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY2),
                &test_mqtt5_property_string_pair2),
            AZ_ERROR_ITEM_NOT_FOUND);

        uint32_t test_mqtt5_property_int_32;
        assert_int_equal(
            az_mqtt5_property_bag_read_int(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_MESSAGE_EXPIRY_INTERVAL,
                &test_mqtt5_property_int_32),
            AZ_ERROR_ITEM_NOT_FOUND);

        uint8_t test_mqtt5_property_int_8;
        assert_int_equal(
            az_mqtt5_property_bag_read_byte(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_PAYLOAD_FORMAT_INDICATOR,
                &test_mqtt5_property_int_8),
            AZ_ERROR_ITEM_NOT_FOUND);

        az_mqtt5_property_binarydata test_mqtt5_property_binary_data
            = AZ_MQTT5_PROPERTY_BINARYDATA_EMPTY;
        assert_int_equal(
            az_mqtt5_property_bag_read_binarydata(
                test_recv_data->properties,
                AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
                &test_mqtt5_property_binary_data),
            AZ_ERROR_ITEM_NOT_FOUND);
      }

      break;

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      ref_disconnect++;
      break;
  }

  return ret;
}

#ifndef AZ_NO_PRECONDITION_CHECKING

ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_mqtt5_policy_init_null_failure(void** state)
{
  SETUP_PRECONDITION_CHECK_TESTS();
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_mqtt5_init(NULL, NULL, NULL));
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_mqtt5_policy_init_success(void** state)
{
  (void)state;

  ref_connack = 0;
  ref_suback = 0;
  ref_puback = 0;
  ref_recv = 0;
  ref_disconnect = 0;
  expect_msg_properties = 0;

  options = az_mqtt5_options_default();
  options.disable_tls = true;

  assert_int_equal(
      _az_hfsm_init(
          &test_inbound_hfsm,
          test_inbound_hfsm_root,
          az_inbound_hfsm_get_parent,
          (az_event_policy*)&test_mqtt5_policy,
          NULL),
      AZ_OK);

  assert_int_equal(
      _az_mqtt5_policy_init(
          &test_mqtt5_policy, &test_mqtt5_client, NULL, NULL, (az_event_policy*)&test_inbound_hfsm),
      AZ_OK);

  assert_int_equal(
      _az_event_pipeline_init(
          &test_event_pipeline,
          (az_event_policy*)&test_mqtt5_policy,
          (az_event_policy*)&test_inbound_hfsm),
      AZ_OK);
}

static void test_az_mqtt5_policy_init_valid_success(void** state)
{
  (void)state;

  assert_int_equal(az_mqtt5_init(&test_mqtt5_client, &test_impl_handle, &options), AZ_OK);

  test_mqtt5_client._internal.platform_mqtt5.pipeline = &test_event_pipeline;
}

static void test_az_mqtt5_policy_outbound_connect_success(void** state)
{
  (void)state;
  ref_connack = 0;

  assert_int_equal(az_mqtt5_outbound_connect(&test_mqtt5_client, &test_mqtt5_connect_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_connack == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_connack, 1);

#ifdef TRANSPORT_MOSQUITTO
  assert_ptr_equal(*test_mqtt5_client._internal.mosquitto_handle, test_impl_handle);
#elif TRANSPORT_PAHO
  assert_ptr_equal(test_mqtt5_client._internal.pahoasync_handle, &test_impl_handle);
#endif
}

static void test_az_mqtt5_policy_outbound_sub_success(void** state)
{
  (void)state;
  ref_suback = 0;

  az_mqtt5_sub_data test_mqtt5_sub_data = {
    .topic_filter = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_TOPIC),
    .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
    .out_id = 0,
    .properties = NULL,
  };

  assert_int_equal(az_mqtt5_outbound_sub(&test_mqtt5_client, &test_mqtt5_sub_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_suback == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_suback, 1);
}

static void test_az_mqtt5_policy_outbound_pub_no_properties_success(void** state)
{
  (void)state;
  ref_puback = 0;
  ref_recv = 0;
  expect_msg_properties = 0;

  az_mqtt5_pub_data test_mqtt5_pub_data = {
    .topic = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_TOPIC),
    .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
    .out_id = 0,
    .payload = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_PAYLOAD),
    .properties = NULL,
  };

  assert_int_equal(az_mqtt5_outbound_pub(&test_mqtt5_client, &test_mqtt5_pub_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while ((ref_puback == 0 || ref_recv == 0) && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_puback, 1);
  assert_true(ref_recv > 0);
}

static void test_az_mqtt5_policy_outbound_pub_properties_success(void** state)
{
  (void)state;
  ref_puback = 0;
  ref_recv = 0;
  expect_msg_properties = 1;

#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* prop = NULL;
#elif TRANSPORT_PAHO
  MQTTProperties prop = MQTTProperties_initializer;
#else // TRANSPORT_NONE
  void* prop = NULL;
#endif

  az_mqtt5_property_bag test_mqtt5_property_bag;
  az_mqtt5_property_string test_mqtt5_property_string
      = az_mqtt5_property_string_create(AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_CONTENT_TYPE));
  az_mqtt5_property_stringpair test_mqtt5_property_string_pair1
      = az_mqtt5_property_stringpair_create(
          AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY1),
          AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_VALUE1));
  az_mqtt5_property_stringpair test_mqtt5_property_string_pair2
      = az_mqtt5_property_stringpair_create(
          AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_KEY2),
          AZ_SPAN_FROM_STR(TEST_MQTT_PROPERTY_STRING_PAIR_VALUE2));
  az_mqtt5_property_binarydata test_mqtt5_property_binary_data
      = { .bindata = AZ_SPAN_FROM_BUFFER(TEST_MQTT_PROPERTY_CORRELATION_DATA) };

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_mqtt5_property_bag, &test_mqtt5_client, &prop), AZ_OK);
  assert_int_equal(
      az_mqtt5_property_bag_append_string(
          &test_mqtt5_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
          &test_mqtt5_property_string),
      AZ_OK);
  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_mqtt5_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          &test_mqtt5_property_string_pair1),
      AZ_OK);
  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_mqtt5_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          &test_mqtt5_property_string_pair2),
      AZ_OK);
  assert_int_equal(
      az_mqtt5_property_bag_append_int(
          &test_mqtt5_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_MESSAGE_EXPIRY_INTERVAL,
          TEST_MQTT_PROPERTY_MESSAGE_EXPIRY_INTERVAL),
      AZ_OK);
  assert_int_equal(
      az_mqtt5_property_bag_append_byte(
          &test_mqtt5_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_PAYLOAD_FORMAT_INDICATOR,
          TEST_MQTT_PROPERTY_PAYLOAD_FORMAT_INDICATOR),
      AZ_OK);
  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_mqtt5_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
          &test_mqtt5_property_binary_data),
      AZ_OK);

  az_mqtt5_pub_data test_mqtt5_pub_data = {
    .topic = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_TOPIC),
    .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
    .out_id = 0,
    .payload = AZ_SPAN_LITERAL_FROM_STR(TEST_MQTT_PAYLOAD),
    .properties = &test_mqtt5_property_bag,
  };

  assert_int_equal(az_mqtt5_outbound_pub(&test_mqtt5_client, &test_mqtt5_pub_data), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while ((ref_puback == 0 || ref_recv == 0) && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_puback, 1);
  assert_true(ref_recv > 0);

#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property_free_all(&prop);
#endif
}

static void test_az_mqtt5_policy_outbound_disconnect_success(void** state)
{
  (void)state;
  ref_disconnect = 0;
  expect_msg_properties = 0;

  assert_int_equal(az_mqtt5_outbound_disconnect(&test_mqtt5_client), AZ_OK);

  int retries = TEST_MAX_RESPONSE_CHECKS;
  while (ref_disconnect == 0 && retries > 0)
  {
    assert_int_equal(az_platform_sleep_msec(TEST_RESPONSE_DELAY_MS), AZ_OK);
    retries--;
  }

  assert_int_equal(ref_disconnect, 1);
}

int test_az_mqtt5_policy()
{
  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_mqtt5_policy_init_null_failure),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_mqtt5_policy_init_success),
    cmocka_unit_test(test_az_mqtt5_policy_init_valid_success),
    cmocka_unit_test(test_az_mqtt5_policy_outbound_connect_success),
    cmocka_unit_test(test_az_mqtt5_policy_outbound_sub_success),
    cmocka_unit_test(test_az_mqtt5_policy_outbound_pub_no_properties_success),
    cmocka_unit_test(test_az_mqtt5_policy_outbound_pub_properties_success),
    cmocka_unit_test(test_az_mqtt5_policy_outbound_disconnect_success),
  };
  return cmocka_run_group_tests_name("az_mqtt5_policy", tests, NULL, NULL);
}
