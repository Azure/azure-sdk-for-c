// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_mqtt5_request.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include <cmocka.h>

// #define TEST_COMMAND_NAME "test_command_name"
// #define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_client_id"
// #define TEST_SERVER_ID "test_server_id"
// #define TEST_CONTENT_TYPE "test_content_type"
// #define TEST_PAYLOAD "test_payload"
#define TEST_CORRELATION_ID "correlation_id"
// #define TEST_STATUS_SUCCESS "200"
// #define TEST_STATUS_FAILURE "500"
// #define TEST_STATUS_MESSAGE "test_status_message"
#define TEST_REQUEST_COMPLETION_TIMEOUT_S 10
#define TEST_PUBLISH_TIMEOUT_S 10

#define TEST_HOSTNAME "test.hostname.com"
// #define TEST_USERNAME "test_username"
// #define TEST_CERTIFICATE "test_certificate"
// #define TEST_KEY "test_key"
// #define TEST_SUBSCRIPTION_TOPIC_FORMAT 
//   "vehicles/{serviceId}/commands/{executorId}/{name}/for/{invokerClientId}"
// #define TEST_REQUEST_TOPIC_FORMAT "vehicles/{serviceId}/commands/{executorId}/{name}"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static _az_event_client mock_client_1;
static _az_hfsm mock_client_hfsm_1;

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

// static az_mqtt5_rpc_client_codec test_rpc_client_codec;
static az_mqtt5_request* test_request;
static _az_event_policy_collection test_request_policy_collection;

// static az_mqtt5_property_bag test_property_bag;
#ifdef TRANSPORT_MOSQUITTO
// static mosquitto_property* test_prop = NULL;
#else // TRANSPORT_PAHO
MQTTAsync test_request_client; // Included so properties can be used for Paho
// static MQTTProperties test_prop = MQTTProperties_initializer;
#endif // TRANSPORT_MOSQUITTO

// static char subscription_topic_buffer[256];
// static char response_topic_buffer[256];
// static char request_topic_buffer[256];

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

// static az_result remove_and_free_command(az_span correlation_id)
// {
//   az_result ret = AZ_OK;
//   az_mqtt5_request *policy_to_remove = NULL;
//   az_mqtt5_rpc_client_remove_req_event_data remove_data = {
//         .correlation_id = &correlation_id,
//         .policy = &policy_to_remove
//       };
  
//   // Get pointers to the data to free and remove the request from the HFSM
//   ret = az_mqtt5_rpc_client_remove_request(&test_rpc_client, &remove_data);
//   if (az_result_succeeded(ret))
//   {
//     // free(az_span_ptr(*remove_data.correlation_id));
//     free(*remove_data.policy);
//   }
//   else
//   {
//     // printf(LOG_APP_ERROR "Failed to remove command '%s' with error: %s\n", az_span_ptr(correlation_id), az_result_to_string(ret));
//   }
//   return ret;
// }

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
    // case AZ_MQTT5_EVENT_SUBACK_RSP:
    //   ref_sub_rsp++;
    //   break;
    // case AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND:
    //   ref_rpc_ready++;
    //   break;
    case AZ_HFSM_EVENT_ERROR:
      ref_rpc_error++;
      break;
    case AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP:
      ref_rpc_err_rsp++;
      break;
    // case AZ_MQTT5_EVENT_RPC_CLIENT_RSP:
    //   ref_rpc_rsp++;
    //   break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_mqtt5_request_init_success(void** state)
{
  (void)state;
  reset_test_counters();

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

#if defined(TRANSPORT_PAHO)
  int test_ret = MQTTAsync_create(
      &test_request_client, TEST_HOSTNAME, TEST_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  (void)test_ret;
#endif // TRANSPORT_PAHO

  assert_int_equal(az_mqtt5_init(&mock_mqtt5, NULL, &mock_mqtt5_options), AZ_OK);
  mock_connection_options = az_mqtt5_connection_options_default();
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

  assert_int_equal(_az_event_policy_collection_init(
        &test_request_policy_collection,
        (az_event_policy*)&mock_client_hfsm_1,
        NULL), AZ_OK);

  test_request = malloc(sizeof(az_mqtt5_request));

  assert_int_equal(
      az_mqtt5_request_init(
          test_request,
          &mock_connection,
          &test_request_policy_collection,
          AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
          AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
          NULL),
      AZ_OK);

  // edit inbound of mqtt policy to go to mock_client
  mock_connection._internal.policy_collection.policy.inbound_policy = mock_client_1.policy;

  // edit inbound of mock_client to go to request policy collection
  mock_client_1.policy->inbound_policy = (az_event_policy*)&test_request_policy_collection;
}

static void test_az_mqtt5_request_set_pub_id_success(void** state)
{
  (void)state;
  reset_test_counters();

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_client_hfsm_1,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);
}

int test_az_mqtt5_request()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_request_init_success),
    cmocka_unit_test(test_az_mqtt5_request_set_pub_id_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_request", tests, NULL, NULL);
}
