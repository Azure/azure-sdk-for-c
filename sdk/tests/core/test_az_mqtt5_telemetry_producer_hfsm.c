// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_producer.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_TELEMETRY_NAME "test_telemetry_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_telemetry_sender_id"
#define TEST_USERNAME "test_username"
#define TEST_PASSWORD "test_password"
#define TEST_CONTENT_TYPE "test_content_type"
#define TEST_PAYLOAD "test_payload"

#define TEST_HOSTNAME "test.hostname.com"
#define TEST_TELEMETRY_TOPIC_FORMAT "vehicles/{modelId}/telemetry/{senderId}/{name}"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static _az_event_client mock_client_1;
static _az_hfsm mock_client_hfsm_1;

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

static az_mqtt5_telemetry_producer_codec test_telemetry_producer_codec;
static az_mqtt5_telemetry_producer test_telemetry_producer;

static az_mqtt5_property_bag test_property_bag;
#ifdef TRANSPORT_MOSQUITTO
static mosquitto_property* test_prop = NULL;
#else // TRANSPORT_PAHO
MQTTAsync test_producer; // Included so properties can be used for Paho
static MQTTProperties test_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

static char telemetry_topic_buffer[256];

static int ref_telemetry_prod_error = 0;
static int ref_pub_rsp = 0;
static int ref_pub_req = 0;
static int ref_telemetry_prod_err_rsp = 0;

/**
 * @brief "Resets all the counters used by these unit tests."
 */
static void reset_test_counters()
{
  ref_telemetry_prod_error = 0;
  ref_pub_rsp = 0;
  ref_pub_req = 0;
  ref_telemetry_prod_err_rsp = 0;
}

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
    case AZ_MQTT5_EVENT_PUBACK_RSP:
      ref_pub_rsp++;
      break;
    case AZ_MQTT5_EVENT_PUB_REQ:
      ref_pub_req++;
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
    case AZ_HFSM_EVENT_ERROR:
      ref_telemetry_prod_error++;
      break;
    case AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_ERROR_RSP:
      ref_telemetry_prod_err_rsp++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_mqtt5_telemetry_producer_init_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

#if defined(TRANSPORT_PAHO)
  int test_ret = MQTTAsync_create(
      &test_producer, TEST_HOSTNAME, TEST_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  (void)test_ret;
#else // TRANSPORT_PAHO
  will_return(__wrap_az_mqtt5_init, AZ_OK);
#endif

  assert_int_equal(az_mqtt5_init(&mock_mqtt5, NULL, &mock_mqtt5_options), AZ_OK);
  mock_connection_options = az_mqtt5_connection_options_default();
  mock_connection_options.disable_sdk_connection_management = true;
  mock_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  mock_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  mock_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  mock_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);

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

  assert_int_equal(az_mqtt5_property_bag_init(&test_property_bag, &mock_mqtt5, &test_prop), AZ_OK);

  az_mqtt5_telemetry_producer_codec_options test_telemetry_producer_codec_options
      = az_mqtt5_telemetry_producer_codec_options_default();
  test_telemetry_producer_codec_options.telemetry_topic_format
      = AZ_SPAN_FROM_STR(TEST_TELEMETRY_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_telemetry_producer_init(
          &test_telemetry_producer,
          &test_telemetry_producer_codec,
          &mock_connection,
          test_property_bag,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_BUFFER(telemetry_topic_buffer),
          AZ_MQTT5_TELEMETRY_DEFAULT_TIMEOUT_SECONDS,
          &test_telemetry_producer_codec_options),
      AZ_OK);

  // edit outbound to go to mock_client
  test_telemetry_producer._internal.subclient.policy->outbound_policy = mock_client_1.policy;
  // edit inbound of mqtt policy to go to mock_client
  mock_connection._internal.policy_collection.policy.inbound_policy = mock_client_1.policy;
}

static void test_az_mqtt5_telemetry_producer_send_begin_qos_1_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5,
          &(az_mqtt5_puback_data){ .id = test_telemetry_producer._internal.pending_pub_id }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_telemetry_producer_double_send_qos_1_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_ERROR_TELEMETRY_PRODUCER_PUB_IN_PROGRESS);

  // no pub should be sent out
  assert_int_equal(ref_pub_req, 1);

  // reset state
  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5,
          &(az_mqtt5_puback_data){ .id = test_telemetry_producer._internal.pending_pub_id }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_telemetry_producer_send_begin_broker_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5,
          &(az_mqtt5_puback_data){ .id = test_telemetry_producer._internal.pending_pub_id,
                                   .reason_code = 135 }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
  assert_int_equal(ref_telemetry_prod_err_rsp, 1);
}

static void test_az_mqtt5_telemetry_producer_send_begin_bad_arg_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_EMPTY,
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_ERROR_ARG);

  assert_int_equal(ref_pub_req, 0);
}

static void test_az_mqtt5_telemetry_producer_send_begin_qos_0_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);
}

static void test_az_mqtt5_telemetry_producer_double_send_qos_0_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  // 2 pubs should be sent out
  assert_int_equal(ref_pub_req, 2);
}

static void test_az_mqtt5_telemetry_producer_send_qos_1_then_send_qos_0_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  // Send QOS 1 telemetry message
  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  // Set QOS to 0
  test_telemetry_data.qos = AZ_MQTT5_QOS_AT_MOST_ONCE;

  // Send QOS 0 telemetry message before QOS 1 message has puback
  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  // 2 pubs should be sent out
  assert_int_equal(ref_pub_req, 2);

  // reset state
  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5,
          &(az_mqtt5_puback_data){ .id = test_telemetry_producer._internal.pending_pub_id }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_telemetry_producer_send_begin_qos_0_bad_arg_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_EMPTY,
          .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_ERROR_ARG);

  assert_int_equal(ref_pub_req, 0);
}

static void test_az_mqtt5_telemetry_producer_send_begin_qos_1_timeout(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_telemetry_producer_send_req_event_data test_telemetry_data
      = { .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
          .telemetry_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .telemetry_name = AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME) };

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, &test_telemetry_data),
      AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      _az_hfsm_send_event(
          &test_telemetry_producer._internal.telemetry_producer_hfsm,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_telemetry_producer._internal.telemetry_producer_timer }),
      AZ_OK);

  assert_int_equal(ref_telemetry_prod_error, 1);
}

static void test_az_mqtt5_telemetry_producer_send_begin_faulted_failure(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(
      az_mqtt5_telemetry_producer_send_begin(&test_telemetry_producer, NULL),
      AZ_ERROR_HFSM_INVALID_STATE);

  assert_int_equal(ref_pub_req, 0);

#if defined(TRANSPORT_PAHO)
  MQTTProperties_free(&test_prop);
  MQTTAsync_destroy(&test_producer);
#endif // TRANSPORT_PAHO
}

int test_az_mqtt5_telemetry_producer()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_init_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_qos_1_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_double_send_qos_1_failure),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_broker_failure),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_bad_arg_failure),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_qos_0_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_double_send_qos_0_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_qos_1_then_send_qos_0_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_qos_0_bad_arg_failure),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_qos_1_timeout),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_send_begin_faulted_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_telemetry_producer", tests, NULL, NULL);
}
