// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_HOSTNAME "test.hostname.com"
#define TEST_CLIENT_ID "test_client_id"
#define TEST_USERNAME "test_username"
#define TEST_PASSWORD "test_password"
#define TEST_PORT 8883
#define TEST_CERTIFICATE "test_certificate"
#define TEST_KEY "test_key"
#define TEST_TOPIC "test_topic"
#define TEST_PAYLOAD "test_payload"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static _az_event_client mock_client_1;
static _az_hfsm mock_client_hfsm_1;

static _az_event_client mock_client_2;
static _az_hfsm mock_client_hfsm_2;

static az_mqtt5_connection test_connection;
static az_mqtt5_connection_options test_connection_options;

static int ref_conn_error = 0;
static int ref_conn_req = 0;
static int ref_conn_rsp = 0;
static int ref_disconn_req = 0;
static int ref_disconn_rsp = 0;
static int ref_sub_req = 0;
static int ref_sub_rsp = 0;
static int ref_pub_rsp = 0;
static int ref_pub_rcv = 0;

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
    case AZ_MQTT5_EVENT_SUBACK_RSP:
      ref_sub_rsp++;
      break;
    case AZ_MQTT5_EVENT_SUB_REQ:
      ref_sub_req++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static az_result test_subclient_policy_2_root(az_event_policy* me, az_event event)
{
  (void)me;
  (void)event;

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
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_mqtt5_connection_disabled_init_success(void** state)
{
  (void)state;
  az_mqtt5_connection test_disabled_connection;
  az_mqtt5_connection_options test_disabled_connection_options;
  az_mqtt5 mock_mqtt_disabled;
  az_mqtt5_options mock_mqtt_options_disabled = { 0 };

  test_disabled_connection_options = az_mqtt5_connection_options_default();

  test_disabled_connection_options.disable_sdk_connection_management = true;
  test_disabled_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_disabled_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_disabled_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_disabled_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_disabled_connection_options.port = TEST_PORT;
  az_mqtt5_x509_client_certificate test_cert = {
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_disabled_connection_options.client_certificates[0] = test_cert;

  assert_int_equal(az_mqtt5_init(&mock_mqtt_disabled, NULL, &mock_mqtt_options_disabled), AZ_OK);

  assert_int_equal(
      az_mqtt5_connection_init(
          &test_disabled_connection,
          NULL,
          &mock_mqtt_disabled,
          test_mqtt_connection_callback,
          &test_disabled_connection_options),
      AZ_OK);
}

static void test_az_mqtt5_connection_enabled_init_success(void** state)
{
  (void)state;

  test_connection_options = az_mqtt5_connection_options_default();

  test_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_connection_options.port = TEST_PORT;
  az_mqtt5_x509_client_certificate test_cert = {
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_connection_options.client_certificates[0] = test_cert;

  assert_int_equal(az_mqtt5_init(&mock_mqtt5, NULL, &mock_mqtt5_options), AZ_OK);

  assert_int_equal(
      az_mqtt5_connection_init(
          &test_connection,
          NULL,
          &mock_mqtt5,
          test_mqtt_connection_callback,
          &test_connection_options),
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

  assert_int_equal(
      _az_hfsm_init(
          &mock_client_hfsm_2,
          test_subclient_policy_2_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);

  mock_client_2.policy = (az_event_policy*)&mock_client_hfsm_2;

  assert_int_equal(
      _az_event_policy_collection_add_client(
          &test_connection._internal.policy_collection, &mock_client_1),
      AZ_OK);

  assert_int_equal(
      _az_event_policy_collection_add_client(
          &test_connection._internal.policy_collection, &mock_client_2),
      AZ_OK);
}

static void test_az_mqtt5_connection_enabled_idle_error_success(void** state)
{
  (void)state;

  ref_conn_error = 0;

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(
          &test_connection._internal.event_pipeline, (az_event){ .type = AZ_HFSM_EVENT_ERROR }),
      AZ_OK);

  assert_int_equal(ref_conn_error, 2);
}

static void test_az_mqtt5_connection_enabled_open_failed(void** state)
{
  // TODO_L: Flawed test. Retry logic is not implemented. Test added for coverage.
  (void)state;

  ref_conn_error = 0;
  ref_conn_req = 0;
  ref_conn_rsp = 0;

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(az_mqtt5_connection_open(&test_connection), AZ_OK);

  assert_int_equal(ref_conn_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt5,
          &(az_mqtt5_connack_data){ .connack_reason = 1, .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp, 2);
  assert_int_equal(ref_conn_error, 1);
}

static void test_az_mqtt5_connection_enabled_open_success(void** state)
{
  (void)state;

  ref_conn_req = 0;
  ref_conn_rsp = 0;

  // will_return(__wrap_az_platform_clock_msec, 0);TODO_L: Add again when retry logic is
  // implemented.

  assert_int_equal(az_mqtt5_connection_open(&test_connection), AZ_OK);

  assert_int_equal(ref_conn_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt5,
          &(az_mqtt5_connack_data){ .connack_reason = 0, .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp, 2);
}

static void test_az_mqtt5_connection_enabled_open_twice_success(void** state)
{
  // TODO_L: Re-work once event filtering is implemented.
  (void)state;

  ref_conn_rsp = 0;

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt5,
          &(az_mqtt5_connack_data){ .connack_reason = 0, .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp, 0);
}

static void test_az_mqtt5_connection_enabled_sub_send_success(void** state)
{
  (void)state;

  ref_sub_rsp = 0;
  ref_sub_req = 0;

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(
          &test_connection._internal.event_pipeline,
          (az_event){ .type = AZ_MQTT5_EVENT_SUB_REQ,
                      .data = (void*)&(az_mqtt5_sub_data){ .out_id = 1,
                                                           .qos = 0,
                                                           .topic_filter
                                                           = AZ_SPAN_FROM_STR(TEST_TOPIC) } }),
      AZ_OK);

  assert_int_equal(ref_sub_req, 1);

  assert_int_equal(az_mqtt5_inbound_suback(&mock_mqtt5, &(az_mqtt5_suback_data){ .id = 1 }), AZ_OK);

  assert_int_equal(ref_sub_rsp, 1);
}

static void test_az_mqtt5_connection_enabled_pub_send_success(void** state)
{
  (void)state;

  ref_pub_rsp = 0;

  assert_int_equal(az_mqtt5_inbound_puback(&mock_mqtt5, &(az_mqtt5_puback_data){ .id = 1 }), AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt5_connection_enabled_pub_recv_success(void** state)
{
  (void)state;

  ref_pub_rcv = 0;

  assert_int_equal(
      az_mqtt5_inbound_recv(
          &mock_mqtt5,
          &(az_mqtt5_recv_data){ .topic = AZ_SPAN_FROM_STR(TEST_TOPIC),
                                 .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                 .qos = 0,
                                 .id = 1 }),
      AZ_OK);

  assert_int_equal(ref_pub_rcv, 1);
}

static void test_az_mqtt5_connection_enabled_close_success(void** state)
{
  (void)state;

  ref_disconn_req = 0;
  ref_disconn_rsp = 0;

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(az_mqtt5_connection_close(&test_connection), AZ_OK);

  assert_int_equal(ref_disconn_req, 1);

  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          &mock_mqtt5,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = true,
                                       .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_disconn_rsp, 2);
}

static void test_az_mqtt5_connection_enabled_open_close_success(void** state)
{
  (void)state;

  ref_conn_req = 0;
  ref_conn_rsp = 0;
  ref_disconn_req = 0;
  ref_disconn_rsp = 0;

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(az_mqtt5_connection_open(&test_connection), AZ_OK);

  assert_int_equal(ref_conn_req, 1);

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(az_mqtt5_connection_close(&test_connection), AZ_OK);

  assert_int_equal(ref_disconn_req, 1);

  assert_int_equal(ref_disconn_rsp, 0);

  assert_int_equal(ref_conn_rsp, 0);
}

int test_az_mqtt5_connection()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_connection_disabled_init_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_init_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_idle_error_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_open_failed),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_open_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_open_twice_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_sub_send_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_pub_send_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_pub_recv_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_close_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_open_close_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt_connection", tests, NULL, NULL);
}
