// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_CLIENT_ID "test_client_id"
#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_SERVER_ID "test_server_id"
#define TEST_CONTENT_TYPE "test_content_type"
#define TEST_PAYLOAD "test_payload"
#define TEST_CORRELATION_ID "correlation_id"
#define TEST_STATUS_SUCCESS "200"
#define TEST_STATUS_FAILURE "500"
#define TEST_STATUS_MESSAGE "test_status_message"

#define TEST_HOSTNAME "test.hostname.com"
#define TEST_USERNAME "test_username"
#define TEST_PASSWORD "test_password"
#define TEST_CERTIFICATE "test_certificate"
#define TEST_KEY "test_key"
#define TEST_SUBSCRIPTION_TOPIC_FORMAT \
  "vehicles/{serviceId}/commands/{executorId}/{name}/for/{invokerClientId}"
#define TEST_REQUEST_TOPIC_FORMAT "vehicles/{serviceId}/commands/{executorId}/{name}"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static _az_event_client mock_client_1;
static _az_hfsm mock_client_hfsm_1;

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

static az_mqtt5_rpc_client_codec test_rpc_client_codec;
static az_mqtt5_rpc_client test_rpc_client;

static az_mqtt5_property_bag test_property_bag;
#ifdef TRANSPORT_MOSQUITTO
static mosquitto_property* test_prop = NULL;
#else // TRANSPORT_PAHO
MQTTAsync test_client; // Included so properties can be used for Paho
static MQTTProperties test_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

static char subscription_topic_buffer[256];
static char response_topic_buffer[256];
static char request_topic_buffer[256];
static char correlation_id_buffer[AZ_MQTT5_RPC_CORRELATION_ID_LENGTH];

static int ref_rpc_error = 0;
static int ref_sub_req = 0;
static int ref_unsub_req = 0;
static int ref_sub_rsp = 0;
static int ref_pub_rsp = 0;
static int ref_pub_req = 0;
static int ref_rpc_ready = 0;
static int ref_unsub_rsp = 0;
static int ref_rpc_err_rsp = 0;
static int ref_rpc_rsp = 0;

/**
 * @brief "Resets all the counters used by these unit tests."
 */
static void reset_test_counters()
{
  ref_rpc_error = 0;
  ref_sub_req = 0;
  ref_unsub_req = 0;
  ref_sub_rsp = 0;
  ref_pub_rsp = 0;
  ref_pub_req = 0;
  ref_rpc_ready = 0;
  ref_unsub_rsp = 0;
  ref_rpc_err_rsp = 0;
  ref_rpc_rsp = 0;
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
    case AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND:
      ref_rpc_ready++;
      break;
    case AZ_HFSM_EVENT_ERROR:
      ref_rpc_error++;
      break;
    case AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP:
      ref_rpc_err_rsp++;
      break;
    case AZ_MQTT5_EVENT_RPC_CLIENT_RSP:
      ref_rpc_rsp++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_mqtt5_rpc_client_init_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

#if defined(TRANSPORT_PAHO)
  int test_ret = MQTTAsync_create(
      &test_client, TEST_HOSTNAME, TEST_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  (void)test_ret;
#endif // TRANSPORT_PAHO

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

  az_mqtt5_rpc_client_codec_options test_client_codec_options
      = az_mqtt5_rpc_client_codec_options_default();
  test_client_codec_options.subscription_topic_format
      = AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC_FORMAT);
  test_client_codec_options.request_topic_format = AZ_SPAN_FROM_STR(TEST_REQUEST_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_init(
          &test_rpc_client,
          &test_rpc_client_codec,
          &mock_connection,
          test_property_bag,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          AZ_SPAN_FROM_BUFFER(correlation_id_buffer),
          AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
          AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
          &test_client_codec_options),
      AZ_OK);

  // edit outbound to go to mock_client
  test_rpc_client._internal.subclient.policy->outbound_policy = mock_client_1.policy;
  // edit inbound of mqtt policy to go to mock_client
  mock_connection._internal.policy_collection.policy.inbound_policy = mock_client_1.policy;
}

static void test_az_mqtt5_rpc_client_invoke_begin_idle_failure(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(
      az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, NULL), AZ_ERROR_HFSM_INVALID_STATE);

  assert_int_equal(ref_pub_req, 0);
}

static void test_az_mqtt5_rpc_client_subscribe_begin_success(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(az_mqtt5_rpc_client_subscribe_begin(&test_rpc_client), AZ_OK);

  // test invalid state calling an invoke before subscribed
  assert_int_equal(
      az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, NULL), AZ_ERROR_HFSM_INVALID_STATE);

  // test no failure on calling subscribe again
  assert_int_equal(az_mqtt5_rpc_client_subscribe_begin(&test_rpc_client), AZ_OK);

  assert_int_equal(ref_sub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_suback(
          &mock_mqtt5,
          &(az_mqtt5_suback_data){ .id = test_rpc_client._internal.pending_subscription_id }),
      AZ_OK);

  assert_int_equal(ref_sub_rsp, 1);
  assert_int_equal(ref_rpc_ready, 1);
}

static void test_az_mqtt5_rpc_client_subscribe_in_ready_failure(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(
      az_mqtt5_rpc_client_subscribe_begin(&test_rpc_client), AZ_ERROR_HFSM_INVALID_STATE);

  assert_int_equal(ref_sub_req, 0);
  assert_int_equal(ref_sub_rsp, 0);
  assert_int_equal(ref_rpc_ready, 0);
}

static void test_az_mqtt5_rpc_client_invoke_begin_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME) };

  assert_int_equal(az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5, &(az_mqtt5_puback_data){ .id = test_rpc_client._internal.pending_pub_id }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_rpc_client_invoke_begin_timeout(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME) };

  assert_int_equal(az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      _az_hfsm_send_event(
          &test_rpc_client._internal.rpc_client_hfsm,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_rpc_client._internal.rpc_client_timer }),
      AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_rpc_client_double_invoke_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME) };

  assert_int_equal(az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data),
      AZ_ERROR_RPC_PUB_IN_PROGRESS);

  // no pub should be sent out
  assert_int_equal(ref_pub_req, 1);

  // reset state
  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5, &(az_mqtt5_puback_data){ .id = test_rpc_client._internal.pending_pub_id }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_rpc_client_invoke_begin_broker_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME) };

  assert_int_equal(az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_puback(
          &mock_mqtt5,
          &(az_mqtt5_puback_data){ .id = test_rpc_client._internal.pending_pub_id,
                                   .reason_code = 135 }),
      AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_rpc_client_invoke_begin_bad_arg_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME) };

  assert_int_equal(
      az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_ERROR_ARG);

  assert_int_equal(ref_pub_req, 0);

  test_command_data.content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE);
  test_command_data.rpc_server_client_id = AZ_SPAN_EMPTY;

  assert_int_equal(
      az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_ERROR_ARG);

  assert_int_equal(ref_pub_req, 0);
}

static void test_az_mqtt5_rpc_client_invoke_begin_no_content_type_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          .content_type = AZ_SPAN_EMPTY,
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
          .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME) };

  assert_int_equal(az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, &test_command_data), AZ_OK);

  assert_int_equal(ref_pub_req, 1);
}

static void test_az_mqtt5_rpc_client_recv_response_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE);
  az_span status_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME);
  az_span status_property_value = AZ_SPAN_FROM_STR(TEST_STATUS_SUCCESS);
  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);

  assert_int_equal(
      az_mqtt5_property_bag_append_string(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, content_type),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_property_key,
          status_property_value),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_rsp, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_recv_fail_response_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span status_message_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_MESSAGE_PROPERTY_NAME);
  az_span status_message_property_value = AZ_SPAN_FROM_STR(TEST_STATUS_MESSAGE);
  az_span status_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME);
  az_span status_property_value = AZ_SPAN_FROM_STR(TEST_STATUS_FAILURE);
  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_message_property_key,
          status_message_property_value),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_property_key,
          status_property_value),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_EMPTY,
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_rsp, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_recv_respose_no_properties_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_recv_data test_resp_data = { .properties = NULL,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_EMPTY,
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_rpc_client_recv_response_no_correlation_data_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_recv_response_no_content_type_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span status_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME);
  az_span status_property_value = AZ_SPAN_FROM_STR(TEST_STATUS_SUCCESS);
  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_property_key,
          status_property_value),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 0);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_recv_response_no_status_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE);
  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);

  assert_int_equal(
      az_mqtt5_property_bag_append_string(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, content_type),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_recv_response_invalid_status_failure(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;

#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span status_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME);
  az_span status_property_value = AZ_SPAN_FROM_STR("invalid_status");
  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_property_key,
          status_property_value),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_recv_response_no_payload_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span status_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME);
  az_span status_property_value = AZ_SPAN_FROM_STR(TEST_STATUS_SUCCESS);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_property_key,
          status_property_value),
      AZ_OK);

  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);
  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_EMPTY,
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 0);
  assert_int_equal(ref_rpc_rsp, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);
}

static void test_az_mqtt5_rpc_client_unsubscribe_begin_success(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(az_mqtt5_rpc_client_unsubscribe_begin(&test_rpc_client), AZ_OK);

  assert_int_equal(ref_unsub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_unsuback(&mock_mqtt5, &(az_mqtt5_unsuback_data){ .id = 3 }), AZ_OK);

  assert_int_equal(ref_unsub_rsp, 1);
}

static void test_az_mqtt5_rpc_client_unsubscribe_begin_idle_success(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(az_mqtt5_rpc_client_unsubscribe_begin(&test_rpc_client), AZ_OK);

  assert_int_equal(ref_unsub_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_unsuback(&mock_mqtt5, &(az_mqtt5_unsuback_data){ .id = 3 }), AZ_OK);

  assert_int_equal(ref_unsub_rsp, 1);
}

static void test_az_mqtt5_rpc_client_recv_response_in_idle_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_mqtt5_property_bag test_resp_property_bag;
#ifdef TRANSPORT_MOSQUITTO
  mosquitto_property* test_resp_prop = NULL;
#else // TRANSPORT_PAHO
  MQTTProperties test_resp_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

  assert_int_equal(
      az_mqtt5_property_bag_init(&test_resp_property_bag, &mock_mqtt5, &test_resp_prop), AZ_OK);

  az_span content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE);
  az_span status_property_key = AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME);
  az_span status_property_value = AZ_SPAN_FROM_STR(TEST_STATUS_SUCCESS);
  az_span correlation_data = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID);

  assert_int_equal(
      az_mqtt5_property_bag_append_string(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, content_type),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_stringpair(
          &test_resp_property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          status_property_key,
          status_property_value),
      AZ_OK);

  assert_int_equal(
      az_mqtt5_property_bag_append_binary(
          &test_resp_property_bag, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data),
      AZ_OK);

  az_mqtt5_recv_data test_resp_data = { .properties = &test_resp_property_bag,
                                        .topic = az_span_create_from_str((char*)az_span_ptr(
                                            test_rpc_client._internal.response_topic_buffer)),
                                        .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                        .qos = AZ_MQTT5_QOS_AT_LEAST_ONCE,
                                        .id = 5 };

  assert_int_equal(az_mqtt5_inbound_recv(&mock_mqtt5, &test_resp_data), AZ_OK);

  assert_int_equal(ref_rpc_rsp, 1);
  assert_int_equal(ref_rpc_ready, 1);

  az_mqtt5_property_bag_clear(&test_resp_property_bag);

  // reset to idle
  reset_test_counters();
  assert_int_equal(az_mqtt5_rpc_client_unsubscribe_begin(&test_rpc_client), AZ_OK);
  assert_int_equal(ref_unsub_req, 1);
  assert_int_equal(
      az_mqtt5_inbound_unsuback(&mock_mqtt5, &(az_mqtt5_unsuback_data){ .id = 3 }), AZ_OK);
  assert_int_equal(ref_unsub_rsp, 1);
}

static void test_az_mqtt5_rpc_client_subscribe_begin_timeout(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(az_mqtt5_rpc_client_subscribe_begin(&test_rpc_client), AZ_OK);

  assert_int_equal(ref_sub_req, 1);

  assert_int_equal(
      _az_hfsm_send_event(
          &test_rpc_client._internal.rpc_client_hfsm,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_rpc_client._internal.rpc_client_timer }),
      AZ_OK);

  assert_int_equal(ref_rpc_error, 1);
  assert_int_equal(ref_sub_rsp, 0);
  assert_int_equal(ref_rpc_ready, 0);
}

static void test_az_mqtt5_rpc_client_invoke_begin_faulted_failure(void** state)
{
  (void)state;
  reset_test_counters();

  assert_int_equal(
      az_mqtt5_rpc_client_invoke_begin(&test_rpc_client, NULL), AZ_ERROR_HFSM_INVALID_STATE);

  assert_int_equal(ref_pub_req, 0);

#if defined(TRANSPORT_PAHO)
  MQTTProperties_free(&test_prop);
  MQTTAsync_destroy(&test_client);
#endif // TRANSPORT_PAHO
}

int test_az_mqtt5_rpc_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_client_init_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_idle_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_subscribe_begin_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_subscribe_in_ready_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_timeout),
    cmocka_unit_test(test_az_mqtt5_rpc_client_double_invoke_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_broker_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_bad_arg_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_no_content_type_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_fail_response_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_respose_no_properties_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_no_correlation_data_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_no_content_type_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_no_status_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_invalid_status_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_no_payload_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_unsubscribe_begin_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_unsubscribe_begin_idle_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_recv_response_in_idle_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_subscribe_begin_timeout),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_faulted_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_client", tests, NULL, NULL);
}
