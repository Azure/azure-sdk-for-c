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

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_client_id"
#define TEST_SERVER_ID "test_server_id"
#define TEST_CONTENT_TYPE "test_content_type"
#define TEST_PAYLOAD "test_payload"
#define TEST_CORRELATION_ID "test_correlation_id"

#define TEST_HOSTNAME "test.hostname.com"
#define TEST_USERNAME "test_username"
#define TEST_CERTIFICATE "test_certificate"
#define TEST_KEY "test_key"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static _az_event_client mock_client_1;
static _az_hfsm mock_client_hfsm_1;

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

static az_mqtt5_rpc_client test_rpc_client;
static az_mqtt5_rpc_client_policy test_rpc_client_policy;

static char subscription_topic_buffer[256];
static char response_topic_buffer[256];
static char request_topic_buffer[256];

static int ref_conn_error = 0;
static int ref_conn_req = 0;
static int ref_conn_rsp = 0;
static int ref_disconn_req = 0;
static int ref_disconn_rsp = 0;
static int ref_sub_req = 0;
static int ref_unsub_req = 0;
static int ref_sub_rsp = 0;
static int ref_pub_rsp = 0;
static int ref_pub_rcv = 0;
static int ref_pub_req = 0;
static int ref_rpc_ready = 0;
static int ref_unsub_rsp = 0;

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
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
      ref_conn_req++;
      break;
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      ref_disconn_req++;
      break;
    case AZ_HFSM_EVENT_ERROR:
      ref_conn_error++;
      break;
    case AZ_MQTT5_EVENT_CONNECT_RSP:
      ref_conn_rsp++;
      break;
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      ref_disconn_rsp++;
      break;
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      ref_pub_rcv++;
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
    case AZ_HFSM_EVENT_TIMEOUT:
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static az_result test_mqtt_connection_callback(az_mqtt5_connection* client, az_event event)
{
  (void)client;
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
      ref_conn_rsp++;
      break;
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      ref_disconn_rsp++;
      break;
    case AZ_MQTT5_EVENT_SUBACK_RSP:
      ref_sub_rsp++;
      break;
    case AZ_EVENT_MQTT5_RPC_CLIENT_READY_IND:
      ref_rpc_ready++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_rpc_client_policy_init_success(void** state)
{
  (void)state;

  assert_int_equal(az_mqtt5_init(&mock_mqtt5, &mock_mqtt5_options), AZ_OK);
  mock_connection_options = az_mqtt5_connection_options_default();
  mock_connection_options.disable_sdk_connection_management = true;

  assert_int_equal(
      az_mqtt5_connection_init(
          &mock_connection,
          NULL,
          &mock_mqtt5,
          test_mqtt_connection_callback,
          &mock_connection_options),
      AZ_OK);

  assert_int_equal(
      _az_hfsm_init(
          &mock_client_hfsm_1,
          test_subclient_policy_1_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);

  mock_client_1.policy = (az_event_policy*)&mock_client_hfsm_1;

  az_mqtt5_property_bag test_property_bag;
  mosquitto_property* test_mosq_prop = NULL;
  assert_int_equal(az_mqtt5_property_bag_init(&test_property_bag, &mock_mqtt5, &test_mosq_prop), AZ_OK);

  assert_int_equal(
      az_rpc_client_policy_init(
          &test_rpc_client_policy,
          &test_rpc_client,
          &mock_connection,
          test_property_bag,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  // edit outbound to go to mock_client
  test_rpc_client_policy._internal.subclient.policy->outbound_policy = mock_client_1.policy;
  // edit inbound of mqtt policy to go to mock_client
  mock_connection._internal.policy_collection.policy.inbound_policy = mock_client_1.policy;

}

static void test_az_mqtt5_rpc_client_subscribe_begin_success(void** state)
{
  (void)state;
  ref_sub_rsp = 0;
  ref_sub_req = 0;
  ref_rpc_ready = 0;

  assert_int_equal(az_mqtt5_rpc_client_subscribe_begin(&test_rpc_client_policy), AZ_OK);

  assert_int_equal(ref_sub_req, 1);

  assert_int_equal(az_mqtt5_inbound_suback(&mock_mqtt5, &(az_mqtt5_suback_data){ .id = test_rpc_client_policy._internal.pending_subscription_id }), AZ_OK);

  assert_int_equal(ref_sub_rsp, 1);
  assert_int_equal(ref_rpc_ready, 1);
}

static void test_az_mqtt5_rpc_client_invoke_begin_success(void** state)
{
  (void)state;
  ref_pub_req = 0;
  ref_pub_rsp = 0;

  az_mqtt5_rpc_client_invoke_req_event_data test_command_data
      = { .correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          .content_type = AZ_SPAN_FROM_STR(TEST_CONTENT_TYPE),
          .rpc_server_client_id = AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          .request_payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD) };

  assert_int_equal(az_mqtt5_rpc_client_invoke_begin(&test_rpc_client_policy, &test_command_data), AZ_OK);

  assert_int_equal(ref_pub_req, 1);

  assert_int_equal(az_mqtt5_inbound_puback(&mock_mqtt5, &(az_mqtt5_puback_data){ .id = test_rpc_client_policy._internal.pending_pub_id }), AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_rpc_client_unsubscribe_begin_success(void** state)
{
  (void)state;
  ref_unsub_req = 0;

  assert_int_equal(az_mqtt5_rpc_client_unsubscribe_begin(&test_rpc_client_policy), AZ_OK);

  assert_int_equal(ref_unsub_req, 1);

  assert_int_equal(az_mqtt5_inbound_unsuback(&mock_mqtt5, &(az_mqtt5_unsuback_data){ .id = 3 }), AZ_OK);

  assert_int_equal(ref_unsub_rsp, 1);
}

int test_az_mqtt5_rpc_client_policy()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_rpc_client_policy_init_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_subscribe_begin_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_invoke_begin_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_unsubscribe_begin_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_client_policy", tests, NULL, NULL);
}
