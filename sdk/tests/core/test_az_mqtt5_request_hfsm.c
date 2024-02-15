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

#define TEST_CLIENT_ID "test_client_id"
#define TEST_CORRELATION_ID "correlation_id"

#define TEST_HOSTNAME "test.hostname.com"
#define TEST_USERNAME "test_username"
#define TEST_PASSWORD "test_password"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

static _az_event_policy_collection test_request_policy_collection;

static int ref_rpc_err_rsp = 0;

/**
 * @brief "Resets all the counters used by these unit tests."
 */
static void reset_test_counters()
{
  ref_rpc_err_rsp = 0;
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

static az_result test_mqtt_connection_callback(
    az_mqtt5_connection* client,
    az_event event,
    void* event_callback_context)
{
  (void)client;
  (void)event_callback_context;
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP:
      ref_rpc_err_rsp++;
      break;
    default:
      assert_true(false);
      break;
  }

  return AZ_OK;
}

static int test_az_mqtt5_request_test_setup(void** state)
{
  (void)state;
  reset_test_counters();

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

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

  assert_int_equal(_az_event_policy_collection_init(
        &test_request_policy_collection,
        NULL,
        NULL), AZ_OK);

  // edit inbound of mqtt policy to go to request policy collection
  mock_connection._internal.policy_collection.policy.inbound_policy = (az_event_policy*)&test_request_policy_collection;

  return 0;
}

static int test_az_mqtt5_request_setup(void** state)
{ 
  reset_test_counters();

  // create request
  az_mqtt5_request* test_request = malloc(sizeof(az_mqtt5_request));
  *state = test_request;

  // request completion timer should be started
  will_return(__wrap_az_platform_timer_create, AZ_OK);
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

  return 0;
}

static int test_az_mqtt5_request_teardown(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;
  // clean up test request
  assert_int_equal(_az_event_policy_collection_remove_client(&test_request_policy_collection, &test_request->_internal.subclient), AZ_OK);
  free(test_request);
  test_request = NULL;
  return 0;
}

static void test_az_mqtt5_request_init_validate_success(void** state)
{
  (void)state;
  reset_test_counters();

  // create request
  az_mqtt5_request* test_request = malloc(sizeof(az_mqtt5_request));

  // request completion timer should be started
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_mqtt5_request_init(
          test_request,
          &mock_connection,
          &test_request_policy_collection,
          AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          3,
          AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
          NULL),
      AZ_OK);

  assert_ptr_equal(test_request->_internal.connection, &mock_connection);
  assert_ptr_equal(test_request->_internal.request_policy_collection, &test_request_policy_collection);
  assert_string_equal(az_span_ptr(test_request->_internal.correlation_id), TEST_CORRELATION_ID);
  assert_int_equal(test_request->_internal.publish_timeout_in_seconds, 3);
  assert_int_equal(test_request->_internal.request_completion_timeout_in_seconds, AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS);

  // cleanup
  assert_int_equal(_az_event_policy_collection_remove_client(&test_request_policy_collection, &test_request->_internal.subclient), AZ_OK);
  free(test_request);
  test_request = NULL;
}

static void test_az_mqtt5_request_init_invalid_args_failure(void** state)
{
  (void)state;
  reset_test_counters();

  // create request
  az_mqtt5_request* test_request = malloc(sizeof(az_mqtt5_request));

  // request completion timer shouldn't be started
  // Init with invalid publish timeout
  assert_int_equal(
      az_mqtt5_request_init(
          test_request,
          &mock_connection,
          &test_request_policy_collection,
          AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
          0,
          AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
          NULL),
      AZ_ERROR_ARG);

  // cleanup
  free(test_request);
  test_request = NULL;
}

static void test_az_mqtt5_request_set_pub_id_different_corr_id_success(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // Timer should not be created. If it is, this test will fail since we don't check for it
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR("1234"), .pub_id = 1} }),
      AZ_OK);
  
  // Check that the pending pub id is still the default value and didn't get set to 1
  assert_int_equal(test_request->_internal.pending_pub_id, 0);
}

static void test_az_mqtt5_request_set_pub_id_success(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // publish timer should be started
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  // Check that the request has been updated with the correct pending pub id
  assert_int_equal(test_request->_internal.pending_pub_id, 1);
}

static void test_az_mqtt5_request_completion_timeout_failure(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_request->_internal.request_completion_timer }),
      AZ_OK);

  // Request should send an error to the application that it has timed out
  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_request_faulted_remove_success(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // Send timeout to trigger transition to faulted state
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_request->_internal.request_completion_timer }),
      AZ_OK);

  // Request should send an error to the application that it has timed out
  assert_int_equal(ref_rpc_err_rsp, 1);

  // In the faulted state, the request should be removable without sending the correlation id
  az_mqtt5_request *policy_to_remove = NULL;
  az_span correlation_id = AZ_SPAN_EMPTY;
  az_mqtt5_rpc_client_remove_req_event_data removal_data = {
                        .correlation_id = &correlation_id,
                        .policy = &policy_to_remove
                      };

  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ,
                      .data = &removal_data }),
      AZ_OK);

  // ensure the proper pointers to be free'd are returned
  assert_ptr_equal(*removal_data.policy, test_request);
  assert_ptr_equal(az_span_ptr(*removal_data.correlation_id), az_span_ptr(test_request->_internal.correlation_id));
}

// TODO: find a way to make sure the correlation id matched and it didn't just get ignored
static void test_az_mqtt5_request_publishing_puback_success(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // "send publish". Publish timer should be started
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 1);

  // Send puback
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_PUBACK_RSP,
                      .data = &(az_mqtt5_puback_data){ .id = 1, .reason_code = 0 } }),
      AZ_OK);

  // Check that pub id is cleared
  assert_int_equal(test_request->_internal.pending_pub_id, 0);

  // no error should be sent to the application on a successful puback
  assert_int_equal(ref_rpc_err_rsp, 0);
}

static void test_az_mqtt5_request_publishing_puback_failure(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // "send publish". Publish timer should be started
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 1);

  // Get puback
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_PUBACK_RSP,
                      .data = &(az_mqtt5_puback_data){ .id = 1, .reason_code = 5 } }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 0);

  // Error should be sent to the application
  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_request_publishing_timeout_failure(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // "send publish"
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 1);

  // On pub timeout, pub id should be cleared to indicate the transition to faulted, and an error should be sent to the application
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_request->_internal.request_pub_timer }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 0);

  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_request_publishing_completion_timeout_failure(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // "send publish"
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 1);

  // On request completion timeout, pub id should be cleared to indicate the transition to faulted, and an error should be sent to the application
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_request->_internal.request_completion_timer }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 0);

  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_request_waiting_completion_timeout_failure(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // "send publish"
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 1);

  // Get puback
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_PUBACK_RSP,
                      .data = &(az_mqtt5_puback_data){ .id = 1, .reason_code = 0 } }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 0);

  // no error should be sent to the application on a successful puback
  assert_int_equal(ref_rpc_err_rsp, 0);

  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT,
                      .data = &test_request->_internal.request_completion_timer }),
      AZ_OK);

  assert_int_equal(ref_rpc_err_rsp, 1);
}

static void test_az_mqtt5_request_full_happy_path_success(void** state)
{
  az_mqtt5_request* test_request = (az_mqtt5_request*)*state;

  // "send publish"
  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_INIT,
                      .data = &(init_event_data){.correlation_id = AZ_SPAN_FROM_STR(TEST_CORRELATION_ID), .pub_id = 1} }),
      AZ_OK);

  assert_int_equal(test_request->_internal.pending_pub_id, 1);

  // Get puback
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_PUBACK_RSP,
                      .data = &(az_mqtt5_puback_data){ .id = 1, .reason_code = 0 } }),
      AZ_OK);

  // no error should be sent to the application on a successful puback
  assert_int_equal(ref_rpc_err_rsp, 0);

  // Send request complete
  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_REQUEST_COMPLETE,
                      .data = &AZ_SPAN_FROM_STR(TEST_CORRELATION_ID) }),
      AZ_OK);

  // Check that request sends the correct information to be free'd from the completed state
  az_mqtt5_request *policy_to_remove = NULL;
  az_mqtt5_rpc_client_remove_req_event_data removal_data = {
                        .correlation_id = &AZ_SPAN_FROM_STR(TEST_CORRELATION_ID),
                        .policy = &policy_to_remove
                      };

  assert_int_equal(
      az_event_policy_send_inbound_event(
          (az_event_policy*)&mock_connection._internal.policy_collection,
          (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ,
                      .data = &removal_data }),
      AZ_OK);

  // ensure the proper pointers to be free'd are returned
  assert_ptr_equal(*removal_data.policy, test_request);
  assert_ptr_equal(az_span_ptr(*removal_data.correlation_id), az_span_ptr(test_request->_internal.correlation_id));

}

int test_az_mqtt5_request()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_request_init_validate_success),
    cmocka_unit_test(test_az_mqtt5_request_init_invalid_args_failure),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_set_pub_id_different_corr_id_success, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_set_pub_id_success, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_completion_timeout_failure, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_faulted_remove_success, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_publishing_puback_success, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_publishing_puback_failure, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_publishing_timeout_failure, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_publishing_completion_timeout_failure, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_waiting_completion_timeout_failure, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
    cmocka_unit_test_setup_teardown(test_az_mqtt5_request_full_happy_path_success, test_az_mqtt5_request_setup, test_az_mqtt5_request_teardown),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_request", tests, test_az_mqtt5_request_test_setup, NULL);
}
