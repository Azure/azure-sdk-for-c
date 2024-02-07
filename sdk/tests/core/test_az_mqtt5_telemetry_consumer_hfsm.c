// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_consumer.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_HOSTNAME "test.hostname.com"
#define TEST_TELEMETRY_NAME "test_telemetry_name"
#define TEST_SERVICE_GROUP_ID "test_service_group_id"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_telemetry_sender_id"
#define TEST_USERNAME "test_username"
#define TEST_PASSWORD "test_password"
#define TEST_CONTENT_TYPE "test_content_type"
#define TEST_PAYLOAD "test_payload"
#define TEST_SUBSCRIPTION_TOPIC_FORMAT "sender/{senderId}/service/{serviceId}/telemetry/{name}"
#define TEST_SUBSCRIPTION_TOPIC "sender/" TEST_SENDER_ID "/service/" TEST_MODEL_ID "/telemetry/+\0"

#define TEST_STATUS_SUCCESS "200"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static _az_event_client mock_client_1;
static _az_hfsm mock_client_hfsm_1;

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

static az_mqtt5_telemetry_consumer_codec test_telemetry_consumer_codec;
static az_mqtt5_telemetry_consumer test_telemetry_consumer;

char telemetry_subscription_topic_buffer[256];

#ifdef TRANSPORT_PAHO
MQTTAsync test_consumer; // Included so properties can be used for Paho
#endif // TRANSPORT_PAHO

static int ref_telemetry_error = 0;
static int ref_sub_req = 0;
static int ref_sub_rsp = 0;
static int ref_telemetry_ind = 0;
static int ref_unsub_req = 0;
static int ref_unsub_rsp = 0;

AZ_INLINE void az_sdk_log_callback(az_log_classification classification, az_span message)
{
  (void)classification;
  (void)message;
}
AZ_INLINE bool az_sdk_log_filter_callback(az_log_classification classification)
{
  (void)classification;
  // Enable all logging.
  return true;
}

static az_event_policy_handler test_subclient_policy_get_parent(
    az_event_policy_handler child_handler)
{
  (void)child_handler;
  return NULL;
}

static az_result test_subclient_policy_1_root(az_event_policy* me, az_event event)
{
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;
    case AZ_HFSM_EVENT_EXIT:
      break;
    case AZ_HFSM_EVENT_ERROR:
      break;
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      break;
    case AZ_MQTT5_EVENT_SUBACK_RSP:
      ref_sub_rsp++;
      break;
    case AZ_MQTT5_EVENT_SUB_REQ:
      ref_sub_req++;
      break;
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
      ref_unsub_rsp++;
      break;
    case AZ_MQTT5_EVENT_UNSUB_REQ:
      ref_unsub_req++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static az_result test_mqtt_connection_callback(
    az_mqtt5_connection* client,
    az_event event,
    void* event_callback_context)
{
  (void)client;
  (void)event_callback_context;
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_SUBACK_RSP:
      ref_sub_rsp++;
      break;
    case AZ_HFSM_EVENT_ERROR:
      ref_telemetry_error++;
      break;
    case AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_IND:
      ref_telemetry_ind++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_mqtt5_telemetry_consumer_init_specific_endpoint_success(void** state)
{
  (void)state;

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

#if defined(TRANSPORT_PAHO)
  int test_ret = MQTTAsync_create(
      &test_consumer, "TEST_HOSTNAME", TEST_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  (void)test_ret;
#endif // TRANSPORT_PAHO

  will_return(__wrap_az_mqtt5_init, AZ_OK);
  assert_int_equal(az_mqtt5_init(&mock_mqtt5, NULL, &mock_mqtt5_options), AZ_OK);

  mock_connection_options = az_mqtt5_connection_options_default();
  mock_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  mock_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  mock_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  mock_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  mock_connection_options.disable_sdk_connection_management = true;

  assert_int_equal(
      az_mqtt5_connection_init(
          &mock_connection,
          NULL,
          &mock_mqtt5,
          test_mqtt_connection_callback,
          &mock_connection_options,
          NULL),
      AZ_OK);

  // mock client policy so we can check what events are being sent
  assert_int_equal(
      _az_hfsm_init(
          &mock_client_hfsm_1,
          test_subclient_policy_1_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);

  mock_client_1.policy = (az_event_policy*)&mock_client_hfsm_1;

  az_mqtt5_telemetry_consumer_codec_options test_telemetry_consumer_codec_options
      = az_mqtt5_telemetry_consumer_codec_options_default();
  test_telemetry_consumer_codec_options.telemetry_topic_format
      = AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC_FORMAT);
  assert_int_equal(
      az_mqtt5_telemetry_consumer_init(
          &test_telemetry_consumer,
          &test_telemetry_consumer_codec,
          &mock_connection,
          AZ_SPAN_FROM_BUFFER(telemetry_subscription_topic_buffer),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_MQTT5_TELEMETRY_DEFAULT_TIMEOUT_SECONDS,
          &test_telemetry_consumer_codec_options),
      AZ_OK);

  // edit outbound to go to mock_client
  test_telemetry_consumer._internal.subclient.policy->outbound_policy = mock_client_1.policy;
  // edit inbound of mqtt policy to go to mock_client
  mock_connection._internal.policy_collection.policy.inbound_policy = mock_client_1.policy;
}

static void test_az_mqtt5_telemetry_consumer_subscribe_specific_endpoint_success(void** state)
{
  (void)state;
  ref_sub_rsp = 0;
  ref_sub_req = 0;

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(az_mqtt5_telemetry_consumer_subscribe_begin(&test_telemetry_consumer), AZ_OK);

  assert_int_equal(ref_sub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_suback(
          &mock_mqtt5,
          &(az_mqtt5_suback_data){ .id
                                   = test_telemetry_consumer._internal.pending_subscription_id }),
      AZ_OK);

  assert_int_equal(ref_sub_rsp, 1);
}

static void test_az_mqtt5_telemetry_consumer_recv_telemetry_specific_endpoint_success(void** state)
{
  (void)state;
  ref_telemetry_ind = 0;

  az_mqtt5_property_bag test_telemetry_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_telemetry_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_telemetry_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO
  assert_int_equal(
      az_mqtt5_property_bag_init(&test_telemetry_property_bag, &mock_mqtt5, &test_telemetry_prop),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_string(
          &test_telemetry_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
          AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE)),
      AZ_OK);

  az_mqtt5_recv_data test_telemetry_data
      = { .properties = &test_telemetry_property_bag,
          .topic = test_telemetry_consumer._internal.subscription_topic,
          .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_telemetry_data), AZ_OK);

  assert_int_equal(ref_telemetry_ind, 1);

#if defined(TRANSPORT_MOSQUITTO)
  mosquitto_property_free_all(&test_telemetry_prop);
#elif defined(TRANSPORT_PAHO)
  MQTTAsync_destroy(&test_consumer);
  MQTTProperties_free(&test_telemetry_prop);
#endif // TRANSPORT_PAHO
}

static void test_az_mqtt5_telemetry_consumer_recv_telemetry_no_content_type_success(void** state)
{
  (void)state;
  ref_telemetry_ind = 0;

  az_mqtt5_property_bag test_telemetry_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_telemetry_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_telemetry_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO
  assert_int_equal(
      az_mqtt5_property_bag_init(&test_telemetry_property_bag, &mock_mqtt5, &test_telemetry_prop),
      AZ_OK);

  az_mqtt5_recv_data test_telemetry_data
      = { .properties = &test_telemetry_property_bag,
          .topic = test_telemetry_consumer._internal.subscription_topic,
          .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_telemetry_data), AZ_OK);

  assert_int_equal(ref_telemetry_ind, 1);

#if defined(TRANSPORT_MOSQUITTO)
  mosquitto_property_free_all(&test_telemetry_prop);
#elif defined(TRANSPORT_PAHO)
  MQTTAsync_destroy(&test_consumer);
  MQTTProperties_free(&test_telemetry_prop);
#endif // TRANSPORT_PAHO
}

static void test_az_mqtt5_telemetry_consumer_unsubscribe_begin_success(void** state)
{
  (void)state;
  ref_unsub_req = 0;
  ref_unsub_rsp = 0;

  assert_int_equal(az_mqtt5_telemetry_consumer_unsubscribe_begin(&test_telemetry_consumer), AZ_OK);

  assert_int_equal(ref_unsub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_unsuback(&mock_mqtt5, &(az_mqtt5_unsuback_data){ .id = 3 }), AZ_OK);

  assert_int_equal(ref_unsub_rsp, 1);
}

static void test_az_mqtt5_telemetry_consumer_subscribe_begin_timeout(void** state)
{
  (void)state;
  ref_sub_req = 0;
  ref_sub_rsp = 0;
  ref_telemetry_error = 0;

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(az_mqtt5_telemetry_consumer_subscribe_begin(&test_telemetry_consumer), AZ_OK);

  assert_int_equal(ref_sub_req, 1);

  assert_int_equal(
      _az_hfsm_send_event(
          &test_telemetry_consumer._internal.telemetry_consumer_hfsm,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_telemetry_consumer._internal.telemetry_consumer_timer }),
      AZ_OK);

  assert_int_equal(ref_telemetry_error, 1);
  assert_int_equal(ref_sub_rsp, 0);
}

static void test_az_mqtt5_telemetry_consumer_subscribe_in_faulted_failure(void** state)
{
  (void)state;
  ref_sub_req = 0;

  assert_int_equal(
      az_mqtt5_telemetry_consumer_subscribe_begin(&test_telemetry_consumer),
      AZ_ERROR_HFSM_INVALID_STATE);

  assert_int_equal(ref_sub_req, 0);
}

int test_az_mqtt5_telemetry_consumer()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_init_specific_endpoint_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_subscribe_specific_endpoint_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_recv_telemetry_specific_endpoint_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_recv_telemetry_no_content_type_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_unsubscribe_begin_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_subscribe_begin_timeout),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_subscribe_in_faulted_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_telemetry_consumer", tests, NULL, NULL);
}
