// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_connection_config.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/iot/az_iot_common.h>

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
static int ref_retry_exhausted = 0;

static az_mqtt5_connection_options test_connection_options;

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
    case AZ_HFSM_EVENT_TIMEOUT:
      break;
    case AZ_MQTT5_EVENT_PUB_REQ:
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
    case AZ_EVENT_MQTT5_CONNECTION_RETRY_EXHAUSTED_IND:
      ref_retry_exhausted++;
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
  ref_retry_exhausted = 0;
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
  test_conn->_internal.connection_timer._internal.pipeline = &test_conn->_internal.event_pipeline;
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

AZ_INLINE void test_az_mqtt5_connection_idle_connecting(az_mqtt5_connection* test_conn)
{
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  int ref_conn_req_prior = ref_conn_req;
  assert_int_equal(az_mqtt5_connection_open(test_conn), AZ_OK);
  assert_int_equal(ref_conn_req_prior + 1, ref_conn_req);
}

AZ_INLINE void test_az_mqtt5_connection_connecting_connected(az_mqtt5* mock_mqtt5)
{
  will_return(__wrap_az_platform_clock_msec, 0);
  int ref_conn_rsp_ok_prior = ref_conn_rsp_ok;

  assert_int_equal(
      az_mqtt5_inbound_connack(
          mock_mqtt5,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_SUCCESS,
                                    .tls_authentication_error = false }),
      AZ_OK);
  // One pass through the policy collection, another through the callback.
  assert_int_equal(ref_conn_rsp_ok_prior + 2, ref_conn_rsp_ok);
}

AZ_INLINE void test_az_mqtt5_connection_connected_connecting(az_mqtt5* mock_mqtt5)
{
  will_return(__wrap_az_platform_clock_msec, 0);
  int ref_disconn_rsp_prior = ref_disconn_rsp;
  int ref_conn_req_prior = ref_conn_req;
  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          mock_mqtt5,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = false,
                                       .tls_authentication_error = false }),
      AZ_OK);
  assert_int_equal(ref_disconn_rsp_prior + 1, ref_disconn_rsp);
  assert_int_equal(ref_conn_req_prior + 1, ref_conn_req);
}

AZ_INLINE void test_az_mqtt5_connection_connected_disconnecting(az_mqtt5_connection* test_conn)
{
  int ref_disconn_req_prior = ref_disconn_req;

  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(az_mqtt5_connection_close(test_conn), AZ_OK);
  assert_int_equal(ref_disconn_req_prior + 1, ref_disconn_req);
}

AZ_INLINE void test_az_mqtt5_connection_disconnecting_idle(az_mqtt5* mock_mqtt5)
{
  int ref_disconn_rsp_prior = ref_disconn_rsp;
  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          mock_mqtt5,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = true,
                                       .tls_authentication_error = false }),
      AZ_OK);
  // One pass through the policy collection, another through the callback.
  assert_int_equal(ref_disconn_rsp_prior + 2, ref_disconn_rsp);
}

AZ_INLINE void test_az_mqtt5_connection_connecting_reconnecttimeout(
    az_mqtt5* mock_mqtt5,
    int32_t connack_reason)
{
  int ref_conn_rsp_err_prior = ref_conn_rsp_err;
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(
      az_mqtt5_inbound_connack(
          mock_mqtt5,
          &(az_mqtt5_connack_data){ .connack_reason = connack_reason,
                                    .tls_authentication_error = false }),
      AZ_OK);
  assert_int_equal(ref_conn_rsp_err_prior + 1, ref_conn_rsp_err);
}

AZ_INLINE void test_az_mqtt5_connection_reconnecttimeout_connecting(az_mqtt5_connection* test_conn)
{
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);

  _az_event_pipeline_timer* timer = &test_conn->_internal.connection_timer;
  az_event timer_event = (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT, .data = timer };

  az_result ret = _az_event_pipeline_post_outbound_event(timer->_internal.pipeline, timer_event);

  if (az_result_failed(ret))
  {
    ret = _az_event_pipeline_post_inbound_event(
        timer->_internal.pipeline,
        (az_event){ .type = AZ_HFSM_EVENT_ERROR,
                    .data = &(az_hfsm_event_data_error){
                        .error_type = ret,
                        .sender_event = timer_event,
                        .sender = timer->_internal.pipeline->_internal.outbound_policy,
                    } });
  }

  if (az_result_failed(ret))
  {
    assert_true(false);
  }
}

static void test_az_mqtt5_connection_options_init(void** state)
{
  (void)state;
  test_connection_options = az_mqtt5_connection_options_default();
  test_connection_options.client_certificate_count = 2;
  test_connection_options.client_certificates[0] = (az_mqtt5_x509_client_certificate){
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_connection_options.client_certificates[1] = (az_mqtt5_x509_client_certificate){
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_connection_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_connection_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_connection_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_connection_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_connection_options.port = TEST_PORT;
}

static void test_az_mqtt5_connection_disabled_init_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_disabled_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_connection_disabled_options = az_mqtt5_connection_options_default();

  test_connection_disabled_options.disable_sdk_connection_management = true;
  test_connection_disabled_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_connection_disabled_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_connection_disabled_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_connection_disabled_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_connection_disabled_options.port = TEST_PORT;
  az_mqtt5_x509_client_certificate test_cert = {
    .cert = AZ_SPAN_FROM_STR(TEST_CERTIFICATE),
    .key = AZ_SPAN_FROM_STR(TEST_KEY),
  };
  test_connection_disabled_options.client_certificates[0] = test_cert;
  test_connection_disabled_options.client_certificate_count = 1;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_disabled_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);
}

static void test_az_mqtt5_connection_enabled_no_client_cert_init_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5_connection_options test_connection_no_certs_options;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;
  test_connection_no_certs_options = az_mqtt5_connection_options_default();

  test_connection_no_certs_options.disable_sdk_connection_management = false;
  test_connection_no_certs_options.hostname = AZ_SPAN_FROM_STR(TEST_HOSTNAME);
  test_connection_no_certs_options.client_id_buffer = AZ_SPAN_FROM_STR(TEST_CLIENT_ID);
  test_connection_no_certs_options.username_buffer = AZ_SPAN_FROM_STR(TEST_USERNAME);
  test_connection_no_certs_options.password_buffer = AZ_SPAN_FROM_STR(TEST_PASSWORD);
  test_connection_no_certs_options.port = TEST_PORT;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_no_certs_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);
}

// Idle
static void test_az_mqtt5_connection_idle_ignore_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  assert_int_equal(az_mqtt5_connection_close(&test_connection), AZ_OK);
  assert_int_equal(ref_disconn_req, 1);
  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          &mock_mqtt,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = true,
                                       .tls_authentication_error = false }),
      AZ_OK);
  assert_int_equal(ref_disconn_rsp, 0);
  assert_int_equal(ref_conn_error, 0);
}

// Idle
static void test_az_mqtt5_connection_root_error(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  assert_int_equal(
      _az_hfsm_send_event(
          (_az_hfsm*)&test_connection._internal.connection_policy,
          (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }),
      AZ_OK);
}

// Idle
static void test_az_mqtt5_connection_root_pub_req_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

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
  assert_int_equal(
      _az_event_pipeline_post_outbound_event(
          &test_connection._internal.event_pipeline,
          (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ,
                      .data = &(az_mqtt5_pub_data){ .topic = AZ_SPAN_FROM_STR(TEST_TOPIC),
                                                    .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
                                                    .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD) } }),
      AZ_OK);
}

// Idle -> Connecting
static void test_az_mqtt5_connection_started_ignore_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  assert_int_equal(az_mqtt5_connection_open(&test_connection), AZ_OK);
  assert_int_equal(ref_conn_req, 2);
}

// Idle -> Connecting -> Faulted
static void test_az_mqtt5_connection_connecting_disconnect_rsp_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          &mock_mqtt,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = true,
                                       .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_disconn_rsp, 0);
  assert_int_equal(ref_conn_error, 1);
}

// Idle -> Connecting -> Disconnecting -> Idle
static void test_az_mqtt5_connection_connecting_close_request_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(az_mqtt5_connection_close(&test_connection), AZ_OK);
  test_az_mqtt5_connection_disconnecting_idle(&mock_mqtt);

  assert_int_equal(ref_disconn_req, 1);
  assert_int_equal(ref_disconn_rsp, 2);
  assert_int_equal(ref_conn_error, 0);
}

// Idle -> Connecting
static void test_az_mqtt5_connection_connecting_pub_request_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(
          &test_connection._internal.event_pipeline,
          (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ,
                      .data = &(az_mqtt5_pub_data){ .topic = AZ_SPAN_FROM_STR(TEST_TOPIC),
                                                    .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
                                                    .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD) } }),
      AZ_ERROR_HFSM_INVALID_STATE);
}

// Idle -> Connecting -> Connected -> Faulted
static void test_az_mqtt5_connection_connected_connect_rsp_ok_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_SUCCESS,
                                    .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_error, 1);
}

// Idle -> Connecting -> Connected -> Faulted
static void test_az_mqtt5_connection_connected_connect_rsp_err_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR,
                                    .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
  assert_int_equal(ref_conn_error, 1);
}

// Idle -> Connecting -> Connected -> Connecting
static void test_az_mqtt5_connection_connected_disconnect_rsp_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);

  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          &mock_mqtt,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = true,
                                       .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
}

// Idle -> Connecting -> Connected
static void test_az_mqtt5_connection_connected_pub_rcv_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);

  assert_int_equal(
      az_mqtt5_inbound_recv(
          &mock_mqtt,
          &(az_mqtt5_recv_data){ .topic = AZ_SPAN_FROM_STR(TEST_TOPIC),
                                 .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
                                 .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD),
                                 .id = 1 }),
      AZ_OK);

  assert_int_equal(ref_pub_rcv, 1);
  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
}

// Idle -> Connecting -> Connected
static void test_az_mqtt5_connection_connected_pub_req_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);

  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(
      _az_event_pipeline_post_outbound_event(
          &test_connection._internal.event_pipeline,
          (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ,
                      .data = &(az_mqtt5_pub_data){ .topic = AZ_SPAN_FROM_STR(TEST_TOPIC),
                                                    .qos = AZ_MQTT5_QOS_AT_MOST_ONCE,
                                                    .payload = AZ_SPAN_FROM_STR(TEST_PAYLOAD) } }),
      AZ_OK);
  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
}

// Idle -> Connecting -> Connected -> Disconnecting
static void test_az_mqtt5_connection_disconnecting_ignore_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);

  assert_int_equal(az_mqtt5_connection_close(&test_connection), AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_error, 0);
  assert_int_equal(ref_disconn_req, 2);
}

// Idle -> Connecting -> Connected -> Disconnecting -> Faulted
static void test_az_mqtt5_connection_disconnecting_connect_rsp_err_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR,
                                    .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
  assert_int_equal(ref_conn_error, 1);
  assert_int_equal(ref_disconn_req, 1);
}

// Idle -> Connecting -> Connected -> Disconnecting -> Idle
static void test_az_mqtt5_connection_disconnecting_connect_rsp_ok_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);

  will_return(__wrap_az_platform_clock_msec, 0);
  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_SUCCESS,
                                    .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
  assert_int_equal(ref_conn_error, 0);
  assert_int_equal(ref_disconn_req, 1);
}

// Idle -> Connecting -> Connected -> Disconnecting -> Faulted
static void test_az_mqtt5_connection_disconnecting_timeout_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);

  _az_event_pipeline_timer* timer = &test_connection._internal.connection_timer;
  az_event timer_event = (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT, .data = timer };

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(timer->_internal.pipeline, timer_event), AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 2);
  assert_int_equal(ref_conn_rsp_err, 0);
  assert_int_equal(ref_conn_error, 1);
  assert_int_equal(ref_disconn_req, 1);
}

// Idle -> Connecting -> ReconnectTimeout
static void test_az_mqtt5_connection_reconnecttimeout_ignore_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);

  az_mqtt5_connection_open(&test_connection);
  assert_int_equal(ref_conn_rsp_ok, 0);
  assert_int_equal(ref_conn_rsp_err, 1);
  assert_int_equal(ref_conn_req, 2);
}

// Idle -> Connecting -> ReconnectTimeout -> Faulted
static void test_az_mqtt5_connection_reconnecttimeout_connect_rsp_ok_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_SUCCESS,
                                    .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 0);
  assert_int_equal(ref_conn_req, 1);
  assert_int_equal(ref_conn_error, 1);
}

// Idle -> Connecting -> ReconnectTimeout -> Faulted
static void test_az_mqtt5_connection_reconnecttimeout_connect_rsp_err_failure(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);

  assert_int_equal(
      az_mqtt5_inbound_connack(
          &mock_mqtt,
          &(az_mqtt5_connack_data){ .connack_reason = AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR,
                                    .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 0);
  assert_int_equal(ref_conn_req, 1);
  assert_int_equal(ref_conn_rsp_err, 1);
  assert_int_equal(ref_conn_error, 1);
}

// Idle -> Connecting -> ReconnectTimeout -> Idle
static void test_az_mqtt5_connection_reconnecttimeout_close_request_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);

  assert_int_equal(az_mqtt5_connection_close(&test_connection), AZ_OK);

  assert_int_equal(ref_conn_rsp_ok, 0);
  assert_int_equal(ref_conn_req, 1);
  assert_int_equal(ref_disconn_req, 1);
}

// Idle -> Connecting -> ReconnectTimeout -> Idle
static void test_az_mqtt5_connection_reconnectimeout_disconnect_rsp_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);

  assert_int_equal(
      az_mqtt5_inbound_disconnect(
          &mock_mqtt,
          &(az_mqtt5_disconnect_data){ .disconnect_requested = true,
                                       .tls_authentication_error = false }),
      AZ_OK);

  assert_int_equal(ref_disconn_rsp, 2);
  assert_int_equal(ref_conn_rsp_ok, 0);
  assert_int_equal(ref_disconn_req, 0);
  assert_int_equal(ref_conn_req, 1);
}

// Idle -> Connecting -> ReconnectTimeout -> Connecting
static void test_az_mqtt5_connection_reconnecttimeout_credential_swap_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(&mock_mqtt, AZ_MQTT5_CONNACK_NOT_AUTHORIZED);
  test_az_mqtt5_connection_reconnecttimeout_connecting(&test_connection);

  assert_int_equal(test_connection._internal.client_certificate_index, 1);
}

// Idle -> Connecting -> ReconnectTimeout -> Connecting -> Idle
static void test_az_mqtt5_connection_reconnecttimeout_retry_exhausted_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);
  test_az_mqtt5_connection_reconnecttimeout_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);
  test_az_mqtt5_connection_reconnecttimeout_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);

  assert_int_equal(ref_retry_exhausted, 1);
  assert_int_equal(ref_conn_req, 1);
  assert_int_equal(ref_conn_rsp_err, 3);
}

// Idle -> Connecting -> Connected -> Disconnecting -> Idle
static void test_az_mqtt5_connection_connect_disconnect_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);
}

// Idle -> Connecting -> ReconnectTimeout -> Connecting -> Connected -> Disconnecting -> Idle
static void test_az_mqtt5_connection_connect_failure_reconnect_disconnect_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);
  test_az_mqtt5_connection_reconnecttimeout_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);
}

// Idle -> Connecting -> ReconnectTimeout -> Idle -> Connecting -> Connected -> Disconnecting
// -> Idle
static void test_az_mqtt5_connection_connect_failure_reconnect_exhaust_idle_success(void** state)
{
  (void)state;
  test_reset_refs();
  az_mqtt5_connection test_connection;
  az_mqtt5 mock_mqtt;
  az_mqtt5_options mock_mqtt_options = { 0 };
  _az_event_client mock_client_1;
  _az_event_client mock_client_2;
  _az_hfsm mock_client_hfsm_1;
  _az_hfsm mock_client_hfsm_2;

  test_az_mqtt5_connection_setup(
      &test_connection,
      &test_connection_options,
      &mock_mqtt,
      &mock_mqtt_options,
      &mock_client_1,
      &mock_client_2,
      &mock_client_hfsm_1,
      &mock_client_hfsm_2);

  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);
  test_az_mqtt5_connection_reconnecttimeout_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);
  test_az_mqtt5_connection_reconnecttimeout_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_reconnecttimeout(
      &mock_mqtt, AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR);
  test_az_mqtt5_connection_idle_connecting(&test_connection);
  test_az_mqtt5_connection_connecting_connected(&mock_mqtt);
  test_az_mqtt5_connection_connected_disconnecting(&test_connection);

  assert_int_equal(ref_retry_exhausted, 1);
  assert_int_equal(ref_conn_req, 2);
  assert_int_equal(ref_conn_rsp_err, 3);
}

int test_az_mqtt5_connection()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_connection_options_init),
    cmocka_unit_test(test_az_mqtt5_connection_disabled_init_success),
    cmocka_unit_test(test_az_mqtt5_connection_enabled_no_client_cert_init_success),
    cmocka_unit_test(test_az_mqtt5_connection_root_error),
    cmocka_unit_test(test_az_mqtt5_connection_root_pub_req_success),
    cmocka_unit_test(test_az_mqtt5_connection_idle_ignore_success),
    cmocka_unit_test(test_az_mqtt5_connection_started_ignore_success),
    cmocka_unit_test(test_az_mqtt5_connection_connecting_disconnect_rsp_failure),
    cmocka_unit_test(test_az_mqtt5_connection_connecting_close_request_success),
    cmocka_unit_test(test_az_mqtt5_connection_connecting_pub_request_success),
    cmocka_unit_test(test_az_mqtt5_connection_connected_connect_rsp_ok_failure),
    cmocka_unit_test(test_az_mqtt5_connection_connected_connect_rsp_err_failure),
    cmocka_unit_test(test_az_mqtt5_connection_connected_disconnect_rsp_success),
    cmocka_unit_test(test_az_mqtt5_connection_connected_pub_rcv_success),
    cmocka_unit_test(test_az_mqtt5_connection_connected_pub_req_success),
    cmocka_unit_test(test_az_mqtt5_connection_disconnecting_ignore_success),
    cmocka_unit_test(test_az_mqtt5_connection_disconnecting_connect_rsp_err_failure),
    cmocka_unit_test(test_az_mqtt5_connection_disconnecting_connect_rsp_ok_success),
    cmocka_unit_test(test_az_mqtt5_connection_disconnecting_timeout_failure),
    cmocka_unit_test(test_az_mqtt5_connection_reconnecttimeout_ignore_success),
    cmocka_unit_test(test_az_mqtt5_connection_reconnecttimeout_connect_rsp_ok_failure),
    cmocka_unit_test(test_az_mqtt5_connection_reconnecttimeout_connect_rsp_err_failure),
    cmocka_unit_test(test_az_mqtt5_connection_reconnecttimeout_close_request_success),
    cmocka_unit_test(test_az_mqtt5_connection_reconnectimeout_disconnect_rsp_success),
    cmocka_unit_test(test_az_mqtt5_connection_reconnecttimeout_credential_swap_success),
    cmocka_unit_test(test_az_mqtt5_connection_reconnecttimeout_retry_exhausted_success),
    cmocka_unit_test(test_az_mqtt5_connection_connect_disconnect_success),
    cmocka_unit_test(test_az_mqtt5_connection_connect_failure_reconnect_disconnect_success),
    cmocka_unit_test(test_az_mqtt5_connection_connect_failure_reconnect_exhaust_idle_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt_connection", tests, NULL, NULL);
}
