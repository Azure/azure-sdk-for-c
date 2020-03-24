// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_pnp_client.h"
#include <az_iot_pnp_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

// PnP Client
#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"
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

#if 0
static const char test_correct_user_name[]
    = "myiothub.azure-devices.net/my_device/?api-version=2018-06-30";
static const char test_correct_user_name_with_module_id[]
    = "myiothub.azure-devices.net/my_device/my_module_id/?api-version=2018-06-30&os=azrtos";
static const char test_correct_client_id[] = "my_device";
static const char test_correct_client_id_with_module_id[] = "my_device/my_module_id";

// Properties
#define TEST_KEY_ONE "key_one"
#define TEST_KEY_TWO "key_two"
#define TEST_VALUE_ONE "value_one"
#define TEST_VALUE_TWO "value_two"
#define TEST_KEY_VALUE_ONE "key_one=value_one"

static const az_span test_key_one = AZ_SPAN_LITERAL_FROM_STR(TEST_KEY_ONE);
static const az_span test_key_two = AZ_SPAN_LITERAL_FROM_STR(TEST_KEY_TWO);
static const az_span test_value_one = AZ_SPAN_LITERAL_FROM_STR(TEST_VALUE_ONE);
static const az_span test_value_two = AZ_SPAN_LITERAL_FROM_STR(TEST_VALUE_TWO);

static const char test_correct_one_key_value[] = "key_one=value_one";
static const char test_correct_two_key_value[] = "key_one=value_one&key_two=value_two";
#endif // 0

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
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_root_interface_name, NULL), AZ_OK);

  assert_non_null(az_span_ptr(client._internal.iot_hub_client._internal.iot_hub_hostname));
  assert_string_equal(TEST_ROOT_INTERFACE_NAME, az_span_ptr(client._internal.root_interface_name));
}

static void test_az_iot_hub_client_init_custom_options_succeed(void** state)
{
  (void)state;

  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.user_agent = test_user_agent;
  options.content_type = test_content_type;
  options.content_encoding = test_content_encoding;

  az_iot_pnp_client client;
  assert_int_equal(
      az_iot_pnp_client_init(&client, test_device_hostname, test_device_id, test_root_interface_name, &options), AZ_OK);

  assert_non_null(az_span_ptr(client._internal.iot_hub_client._internal.iot_hub_hostname));
  assert_string_equal(TEST_USER_AGENT, az_span_ptr(client._internal.iot_hub_client._internal.options.user_agent));
  assert_string_equal(TEST_ROOT_INTERFACE_NAME, az_span_ptr(client._internal.root_interface_name));
  assert_string_equal(TEST_CONTENT_TYPE, az_span_ptr(client._internal.content_type));
  assert_string_equal(TEST_CONTENT_ENCODING, az_span_ptr(client._internal.content_encoding));
}



int test_iot_pnp_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_pnp_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_pnp_client_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_custom_options_succeed),
  };
  return cmocka_run_group_tests_name("az_iot_pnp_client", tests, NULL, NULL);
}
