// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"

static az_mqtt5 mock_mqtt5;
static az_mqtt5_options mock_mqtt5_options = { 0 };

static az_mqtt5_connection mock_connection;
static az_mqtt5_connection_options mock_connection_options = { 0 };

static az_mqtt5_rpc_server test_rpc_server;
static az_mqtt5_rpc_server_options test_rpc_server_options;

static int ref_conn_rsp = 0;
static int ref_disconn_rsp = 0;

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

static void test_az_rpc_server_init_success(void** state)
{
  (void)state;

  assert_int_equal(az_mqtt5_init(&mock_mqtt5, &mock_mqtt5_options), AZ_OK);

  assert_int_equal(
      az_mqtt5_connection_init(
          &mock_connection,
          NULL,
          &mock_mqtt5,
          test_mqtt_connection_callback,
          &mock_connection_options),
      AZ_OK);

  az_mqtt5_property_bag test_property_bag;
  assert_int_equal(az_mqtt5_property_bag_init(&test_property_bag, &mock_mqtt5, NULL), AZ_OK);
  char sub_topic_buffer[256];
  test_rpc_server_options
      = (az_mqtt5_rpc_server_options){ .sub_topic = AZ_SPAN_FROM_BUFFER(sub_topic_buffer),
                                       .command_name = AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
                                       .model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID) };

  az_mqtt5_rpc_server_memory test_rpc_server_memory
      = (az_mqtt5_rpc_server_memory){ .property_bag = test_property_bag };

  assert_int_equal(
      az_rpc_server_init(
          &test_rpc_server, &mock_connection, &test_rpc_server_memory, &test_rpc_server_options),
      AZ_OK);
}

int test_az_mqtt5_rpc_server()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_rpc_server_init_success),

  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_server", tests, NULL, NULL);
}
