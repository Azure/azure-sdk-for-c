// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_pnp_client.h"
#include <az_iot_pnp_client.h>
#include <az_span.h>

#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

// PnP Client
#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_ROOT_INTERFACE_NAME "my_root_interface_name"
#define TEST_USER_AGENT "os=azrtos"
#define TEST_CONTENT_TYPE "my_content_type"
#define TEST_CONTENT_ENCODING "my_content_encoding"

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_root_interface_name = AZ_SPAN_LITERAL_FROM_STR(TEST_ROOT_INTERFACE_NAME);
static const az_span test_user_agent = AZ_SPAN_LITERAL_FROM_STR(TEST_USER_AGENT);
static const az_span test_content_type = AZ_SPAN_LITERAL_FROM_STR(TEST_CONTENT_TYPE);
static const az_span test_content_encoding = AZ_SPAN_LITERAL_FROM_STR(TEST_CONTENT_ENCODING);

static const char test_correct_pnp_user_name[]
    = "myiothub.azure-devices.net/my_device/"
      "?api-version=2018-06-30&digital-twin-model-id=" TEST_ROOT_INTERFACE_NAME;

static const char test_correct_pnp_user_name_with_user_agent[]
    = "myiothub.azure-devices.net/my_device/?api-version=2018-06-30&" TEST_USER_AGENT
      "&digital-twin-model-id=" TEST_ROOT_INTERFACE_NAME;

#ifndef NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void test_az_iot_pnp_client_init_NULL_client_fails(void** state)
{
  (void)state;

  assert_precondition_checked(az_iot_pnp_client_init(
      NULL, test_device_hostname, test_device_id, test_root_interface_name, NULL));
}

static void test_az_iot_pnp_client_init_empty_iot_hub_hostname_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_precondition_checked(az_iot_pnp_client_init(
      &client, AZ_SPAN_NULL, test_device_id, test_root_interface_name, NULL));
}

static void test_az_iot_pnp_client_init_empty_device_id_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_precondition_checked(az_iot_pnp_client_init(
      &client, test_device_hostname, AZ_SPAN_NULL, test_root_interface_name, NULL));
}

static void test_az_iot_pnp_client_init_empty_root_interface_name_fails(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_precondition_checked(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, AZ_SPAN_NULL, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_pnp_client_get_default_options_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  assert_true(az_span_is_content_equal(options.user_agent, AZ_SPAN_NULL));
  assert_true(az_span_is_content_equal(options.content_encoding, AZ_SPAN_NULL));
  assert_true(az_span_is_content_equal(options.content_type, AZ_SPAN_NULL));
}

static void test_az_iot_pnp_client_init_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  assert_non_null(az_span_ptr(client._internal.iot_hub_client._internal.iot_hub_hostname));
  assert_string_equal(TEST_ROOT_INTERFACE_NAME, az_span_ptr(client._internal.root_interface_name));
}

static void test_az_iot_pnp_client_init_custom_options_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.user_agent = test_user_agent;
  options.content_type = test_content_type;
  options.content_encoding = test_content_encoding;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  assert_non_null(az_span_ptr(client._internal.iot_hub_client._internal.iot_hub_hostname));
  assert_string_equal(
      TEST_USER_AGENT, az_span_ptr(client._internal.iot_hub_client._internal.options.user_agent));
  assert_string_equal(TEST_ROOT_INTERFACE_NAME, az_span_ptr(client._internal.root_interface_name));
  assert_string_equal(TEST_CONTENT_TYPE, az_span_ptr(client._internal.options.content_type));
  assert_string_equal(
      TEST_CONTENT_ENCODING, az_span_ptr(client._internal.options.content_encoding));
}

static void test_az_iot_pnp_client_get_user_name_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  uint8_t test_span_buffer[_az_COUNTOF(test_correct_pnp_user_name) - 1];
  az_span test_span = az_span_init(test_span_buffer, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_pnp_client_get_user_name(&client, test_span, &test_span), AZ_OK);

  assert_int_equal(az_span_size(test_span), _az_COUNTOF(test_correct_pnp_user_name) - 1);
  assert_memory_equal(
      test_correct_pnp_user_name,
      az_span_ptr(test_span),
      _az_COUNTOF(test_correct_pnp_user_name) - 1);
}

static void test_az_iot_pnp_client_get_user_name_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, NULL),
      AZ_OK);

  uint8_t test_span_buffer[_az_COUNTOF(test_correct_pnp_user_name) - 2];
  az_span test_span = az_span_init(test_span_buffer, _az_COUNTOF(test_span_buffer));
  assert_int_equal(
      az_iot_pnp_client_get_user_name(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_pnp_client_get_user_name_user_options_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  az_iot_pnp_client_options options;
  options.user_agent = test_user_agent;
  options.content_type = test_content_type;
  options.content_encoding = test_content_encoding;

  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  uint8_t test_span_buffer[_az_COUNTOF(test_correct_pnp_user_name_with_user_agent) - 1];
  az_span test_span = az_span_init(test_span_buffer, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_pnp_client_get_user_name(&client, test_span, &test_span), AZ_OK);

  assert_int_equal(
      az_span_size(test_span), _az_COUNTOF(test_correct_pnp_user_name_with_user_agent) - 1);
  assert_memory_equal(
      test_correct_pnp_user_name_with_user_agent,
      az_span_ptr(test_span),
      _az_COUNTOF(test_correct_pnp_user_name_with_user_agent) - 1);
}

static void test_az_iot_pnp_client_get_user_name_user_options_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_pnp_client client;
  az_iot_pnp_client_options options;
  options.user_agent = test_user_agent;
  options.content_type = test_content_type;
  options.content_encoding = test_content_encoding;

  assert_int_equal(
      az_iot_pnp_client_init(
          &client, test_device_hostname, test_device_id, test_root_interface_name, &options),
      AZ_OK);

  uint8_t test_span_buffer[_az_COUNTOF(test_correct_pnp_user_name_with_user_agent) - 2];
  az_span test_span = az_span_init(test_span_buffer, _az_COUNTOF(test_span_buffer));
  assert_int_equal(
      az_iot_pnp_client_get_user_name(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

// TODO: Add tests for inline functions (e.g. az_iot_pnp_client_get_id).  Will add them prior to
// merge if guidance is available.  Otherwise will open separate GitHub bug to not block merge
// and record number here.

int test_iot_pnp_client()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_pnp_client_init_NULL_client_fails),
    cmocka_unit_test(test_az_iot_pnp_client_init_empty_iot_hub_hostname_fails),
    cmocka_unit_test(test_az_iot_pnp_client_init_empty_device_id_fails),
    cmocka_unit_test(test_az_iot_pnp_client_init_empty_root_interface_name_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_pnp_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_init_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_init_custom_options_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_get_user_name_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_get_user_name_small_buffer_fail),
    cmocka_unit_test(test_az_iot_pnp_client_get_user_name_user_options_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_get_user_name_user_options_small_buffer_fail),
  };
  return cmocka_run_group_tests_name("az_iot_pnp_client", tests, NULL, NULL);
}
