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

#define CONNACK_SUCCESS 0
#define CONNACK_UNSPECIFIED_ERROR 128

AZ_NODISCARD az_result __wrap_az_platform_timer_create(
    _az_platform_timer* out_timer,
    _az_platform_timer_callback callback,
    void* callback_context);
AZ_NODISCARD az_result __wrap_az_platform_timer_create(
    _az_platform_timer* out_timer,
    _az_platform_timer_callback callback,
    void* callback_context)
{
  _az_PRECONDITION_NOT_NULL(out_timer);
  _az_PRECONDITION_NOT_NULL(callback);
  out_timer->_internal.platform_timer._internal.callback = callback;
  out_timer->_internal.platform_timer._internal.callback_context = callback_context;
  return AZ_OK;
}

AZ_NODISCARD az_result
__wrap_az_platform_timer_start(_az_platform_timer* out_timer, int32_t milliseconds);
AZ_NODISCARD az_result
__wrap_az_platform_timer_start(_az_platform_timer* out_timer, int32_t milliseconds)
{
  _az_PRECONDITION_NOT_NULL(out_timer);
  _az_PRECONDITION_RANGE(0, milliseconds, INT32_MAX - 1);
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)out_timer;
  (void)milliseconds;
#endif // AZ_NO_PRECONDITION_CHECKING
  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_platform_timer_destroy(_az_platform_timer* out_timer);
AZ_NODISCARD az_result __wrap_az_platform_timer_destroy(_az_platform_timer* out_timer)
{
  _az_PRECONDITION_NOT_NULL(out_timer);
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)out_timer;
#endif // AZ_NO_PRECONDITION_CHECKING
  return AZ_OK;
}

static int ref_conn_error = 0;
static int ref_conn_req = 0;
static int ref_conn_rsp_ok = 0;
static int ref_conn_rsp_err = 0;
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
    {
      az_mqtt5_connack_data* data = (az_mqtt5_connack_data*)event.data;

      if (data->connack_reason == 0)
      {
        ref_conn_rsp_ok++;
      }
      else
      {
        ref_conn_rsp_err++;
      }
      break;
    }
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

static az_result test_mqtt_connection_callback(
    az_mqtt5_connection* client,
    az_event event,
    void* callback_context)
{
  (void)client;
  (void)callback_context;
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* data = (az_mqtt5_connack_data*)event.data;

      if (data->connack_reason == 0)
      {
        ref_conn_rsp_ok++;
      }
      else
      {
        ref_conn_rsp_err++;
      }
      break;
    }
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      ref_disconn_rsp++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

AZ_INLINE void test_reset_refs()
{
  ref_conn_error = 0;
  ref_conn_req = 0;
  ref_conn_rsp_ok = 0;
  ref_conn_rsp_err = 0;
  ref_disconn_req = 0;
  ref_disconn_rsp = 0;
  ref_sub_req = 0;
  ref_sub_rsp = 0;
  ref_pub_rsp = 0;
  ref_pub_rcv = 0;
}

AZ_INLINE void test_az_mqtt5_connection_setup(
    az_mqtt5_connection* test_conn,
    az_mqtt5_connection_options* test_conn_options,
    az_mqtt5* mock_mqtt5,
    const az_mqtt5_options* mock_mqtt5_options,
    _az_event_client* mock_client_1,
    _az_event_client* mock_client_2,
    _az_hfsm* mock_client_hfsm_1,
    _az_hfsm* mock_client_hfsm_2)
{
  assert_int_equal(az_mqtt5_init(mock_mqtt5, NULL, mock_mqtt5_options), AZ_OK);
  assert_int_equal(
      az_mqtt5_connection_init(
          test_conn, NULL, mock_mqtt5, test_mqtt_connection_callback, test_conn_options, NULL),
      AZ_OK);
  assert_int_equal(
      _az_hfsm_init(
          mock_client_hfsm_1,
          test_subclient_policy_1_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);
  mock_client_1->policy = (az_event_policy*)mock_client_hfsm_1;
  assert_int_equal(
      _az_hfsm_init(
          mock_client_hfsm_2,
          test_subclient_policy_2_root,
          test_subclient_policy_get_parent,
          NULL,
          NULL),
      AZ_OK);
  mock_client_2->policy = (az_event_policy*)mock_client_hfsm_2;
  assert_int_equal(
      _az_event_policy_collection_add_client(
          &test_conn->_internal.policy_collection, mock_client_1),
      AZ_OK);
  assert_int_equal(
      _az_event_policy_collection_add_client(
          &test_conn->_internal.policy_collection, mock_client_2),
      AZ_OK);
}

static void test_az_mqtt5_connection_disabled_init_success(void** state)
{
  (void)state;
  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_connection_options = az_mqtt5_connection_options_default();

  test_connection_options.disable_sdk_connection_management = true;
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
  test_connection_options.client_certificate_count = 1;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);
}

static void test_az_mqtt5_connection_enabled_no_client_cert_success(void** state)
{
  (void)state;
  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;
  test_connection_options = az_mqtt5_connection_options_default();

  test_connection_options.disable_sdk_connection_management = false;
  test_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_connection_options.port = TEST_PORT;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);
}

static void test_az_mqtt5_connection_enabled_init_success(void** state)
{
  (void)state;
  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;
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
  test_connection_options.client_certificate_count = 1;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);
}

static void test_az_mqtt5_connection_open_idle_failure(void** state)
{
  (void)state;
  test_reset_refs();

  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;
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
  test_connection_options.client_certificate_count = 1;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(az_mqtt5_connection_open(&test_connection), AZ_OK);
  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = CONNACK_UNSPECIFIED_ERROR,
                                    .tls_authentication_error = false }),
      AZ_OK);
  assert_int_equal(ref_conn_req, 1);
  assert_int_equal(ref_conn_rsp_err, 1);
}

static void test_az_mqtt5_connection_open_idle_success(void** state)
{
  (void)state;
  test_reset_refs();

  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;
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
  test_connection_options.client_certificate_count = 1;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(az_mqtt5_connection_open(&test_connection), AZ_OK);
  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = CONNACK_SUCCESS,
                                    .tls_authentication_error = false }),
      AZ_OK);
  assert_int_equal(ref_conn_req, 1);
  assert_int_equal(ref_conn_rsp_ok, 2);
}

int test_az_mqtt5_connection()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_connection_disabled_init_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_no_client_cert_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_init_success),
    cmocka_unit_test(test_az_mqtt5_connection_open_idle_failure),
    cmocka_unit_test(test_az_mqtt5_connection_open_idle_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt_connection", tests, NULL, NULL);
}
