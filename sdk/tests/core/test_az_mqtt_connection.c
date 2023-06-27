// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt_connection.h>
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

static az_mqtt mock_mqtt;
static az_mqtt_options mock_mqtt_options = { 0 };

static _az_event_client mock_client;
static _az_hfsm mock_policy_collection;

static az_mqtt_connection test_connection;
static az_mqtt_connection_options test_connection_options;

static int ref_conn_req = 0;
static int ref_conn_rsp = 0;
static int ref_disconn_req = 0;
static int ref_disconn_rsp = 0;
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
    case AZ_EVENT_MQTT_CONNECTION_OPEN_REQ:
      ref_conn_req++;
      break;
    case AZ_EVENT_MQTT_CONNECTION_CLOSE_REQ:
      ref_disconn_req++;
      break;
    case AZ_HFSM_EVENT_ENTRY:
      break;
    case AZ_HFSM_EVENT_EXIT:
      break;
    case AZ_HFSM_EVENT_ERROR:
      break;
    case AZ_MQTT_EVENT_CONNECT_RSP:
      ref_conn_rsp++;
      break;
    case AZ_MQTT_EVENT_DISCONNECT_RSP:
      ref_disconn_rsp++;
      break;
    case AZ_MQTT_EVENT_PUB_RECV_IND:
      ref_pub_rcv++;
      break;
    case AZ_MQTT_EVENT_PUBACK_RSP:
      ref_pub_rsp++;
      break;
    case AZ_MQTT_EVENT_SUBACK_RSP:
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static az_result test_mqtt_connection_callback(az_mqtt_connection* client, az_event event)
{
  (void)client;
  switch (event.type)
  {
    case AZ_MQTT_EVENT_CONNECT_RSP:
      ref_conn_rsp++;
      break;
    case AZ_MQTT_EVENT_DISCONNECT_RSP:
      ref_disconn_rsp++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static void test_az_mqtt_connection_disabled_init_success(void** state)
{
  (void) state;
  az_mqtt_connection test_disabled_connection;
  az_mqtt_connection_options test_disabled_connection_options;
  az_mqtt mock_mqtt_disabled;
  az_mqtt_options mock_mqtt_options_disabled = { 0 };
  
  
  test_disabled_connection_options = az_mqtt_connection_options_default();

  test_disabled_connection_options.disable_sdk_connection_management = true;
  test_disabled_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_disabled_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_disabled_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_disabled_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_disabled_connection_options.port = TEST_PORT;
  az_mqtt_x509_client_certificate test_cert = {
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_disabled_connection_options.client_certificates[0] = test_cert;
  
  assert_int_equal(az_mqtt_init(&mock_mqtt_disabled, &mock_mqtt_options_disabled), AZ_OK);
  assert_int_equal(
      _az_hfsm_init(
          &mock_policy_collection,
          test_subclient_policy_1_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);

  assert_int_equal(
    az_mqtt_connection_init(
        &test_disabled_connection,
        NULL,
        &mock_mqtt_disabled,
        test_mqtt_connection_callback,
        &test_disabled_connection_options),
    AZ_OK);
  
}

static void test_az_mqtt_connection_enabled_init_success(void** state)
{
  (void)state;

  test_connection_options = az_mqtt_connection_options_default();

  test_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_connection_options.port = TEST_PORT;
  az_mqtt_x509_client_certificate test_cert = {
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_connection_options.client_certificates[0] = test_cert;

  assert_int_equal(az_mqtt_init(&mock_mqtt, &mock_mqtt_options), AZ_OK);
  assert_int_equal(
      _az_hfsm_init(
          &mock_policy_collection,
          test_subclient_policy_1_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);

  mock_client.policy = (az_event_policy*)&mock_policy_collection;

  assert_int_equal(
      az_mqtt_connection_init(
          &test_connection,
          NULL,
          &mock_mqtt,
          test_mqtt_connection_callback,
          &test_connection_options),
      AZ_OK);

  assert_int_equal(
      _az_event_policy_collection_add_client(
          &test_connection._internal.policy_collection, &mock_client),
      AZ_OK);
}

static void test_az_mqtt_connection_enabled_open_success(void** state)
{
  (void)state;

  ref_conn_req = 0;
  ref_conn_rsp = 0;

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(az_mqtt_connection_open(&test_connection), AZ_OK);

  assert_int_equal(ref_conn_req, 1);

  assert_int_equal(
      az_mqtt_inbound_connack(
          &mock_mqtt,
          &(az_mqtt_connack_data){ .connack_reason = 0, .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp, 2);
}

static void test_az_mqtt_connection_enabled_pub_send_success(void** state)
{
  (void)state;

  ref_pub_rsp = 0;

  assert_int_equal(az_mqtt_inbound_puback(&mock_mqtt, &(az_mqtt_puback_data){ .id = 1 }), AZ_OK);

  assert_int_equal(ref_pub_rsp, 1);
}

static void test_az_mqtt_connection_enabled_pub_recv_success(void** state)
{
  (void)state;

  ref_pub_rcv = 0;

  assert_int_equal(
      az_mqtt_inbound_recv(
          &mock_mqtt,
          &(az_mqtt_recv_data){ .topic = AZ_SPAN_FROM_STR(TEST_TOPIC),
                                .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                .qos = 0,
                                .id = 1 }),
      AZ_OK);

  assert_int_equal(ref_pub_rcv, 1);
}

static void test_az_mqtt_connection_enabled_close_success(void** state)
{
  (void)state;

  ref_disconn_req = 0;
  ref_disconn_rsp = 0;

  will_return(__wrap_az_platform_clock_msec, 0);

  assert_int_equal(az_mqtt_connection_close(&test_connection), AZ_OK);

  assert_int_equal(ref_disconn_req, 1);

  assert_int_equal(
      az_mqtt_inbound_disconnect(
          &mock_mqtt,
          &(az_mqtt_disconnect_data){ .disconnect_requested = true,
                                      .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_disconn_rsp, 2);
}

int test_az_mqtt_connection()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt_connection_disabled_init_success),
    cmocka_unit_test(test_az_mqtt_connection_enabled_init_success),
    cmocka_unit_test(test_az_mqtt_connection_enabled_open_success),
    cmocka_unit_test(test_az_mqtt_connection_enabled_pub_send_success),
    cmocka_unit_test(test_az_mqtt_connection_enabled_pub_recv_success),
    cmocka_unit_test(test_az_mqtt_connection_enabled_close_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt_connection", tests, NULL, NULL);
}
