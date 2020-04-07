// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_hub_client.h>
#include <az_span.h>
#include <az_test_span.h>

#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 128

// Hub Client
#define TEST_DEVICE_ID_STR "my_device"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"
#define TEST_USER_AGENT "os=azrtos"

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);

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

#ifndef NO_PRECONDITION_CHECKING

enable_precondition_check_tests()

static void test_az_iot_hub_client_init_NULL_client_fails(void** state)
{
  (void)state;

  assert_precondition_checked(
      az_iot_hub_client_init(NULL, test_device_id, test_device_hostname, NULL));
}

static void test_az_iot_hub_client_init_NULL_device_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  assert_precondition_checked(
      az_iot_hub_client_init(&client, AZ_SPAN_NULL, test_device_hostname, NULL));
}

static void test_az_iot_hub_client_init_NULL_device_hostname_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  assert_precondition_checked(
      az_iot_hub_client_init(&client, test_device_id, AZ_SPAN_NULL, NULL));
}

static void test_az_iot_hub_client_user_name_get_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_user_name_get(NULL, test_span, &test_span));
}

static void test_az_iot_hub_client_user_name_get_NULL_input_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_user_name_get(&client, AZ_SPAN_NULL, &test_span));
}

static void test_az_iot_hub_client_user_name_get_NULL_output_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_user_name_get(&client, test_span, NULL));
}

static void test_az_iot_hub_client_id_get_NULL_client_fails(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_id_get(NULL, test_span, &test_span));
}

static void test_az_iot_hub_client_id_get_NULL_input_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_id_get(&client, AZ_SPAN_NULL, &test_span));
}

static void test_az_iot_hub_client_id_get_NULL_output_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_id_get(&client, test_span, NULL));
}

static void test_az_iot_hub_client_properties_init_NULL_props_fails(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_precondition_checked(az_iot_hub_client_properties_init(NULL, test_span));
}

static void test_az_iot_hub_client_properties_init_NULL_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  assert_precondition_checked(az_iot_hub_client_properties_init(&props, AZ_SPAN_NULL));
}

static void test_az_iot_hub_client_properties_append_get_NULL_props_fails(void** state)
{
  (void)state;

  assert_precondition_checked(az_iot_hub_client_properties_append(NULL, test_key_one, test_value_one));
}

static void test_az_iot_hub_client_properties_append_NULL_name_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  assert_precondition_checked(az_iot_hub_client_properties_append(&props, AZ_SPAN_NULL, test_value_one));
}

static void test_az_iot_hub_client_properties_append_NULL_value_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  assert_precondition_checked(az_iot_hub_client_properties_append(&props, test_key_one, AZ_SPAN_NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_get_default_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_span_is_content_equal(options.module_id, AZ_SPAN_NULL));
  assert_true(az_span_is_content_equal(options.user_agent, AZ_SPAN_NULL));
}

static void test_az_iot_hub_client_init_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  assert_memory_equal(
      TEST_DEVICE_ID_STR,
      az_span_ptr(client._internal.device_id),
      _az_COUNTOF(TEST_DEVICE_ID_STR) - 1);
  assert_memory_equal(
      TEST_DEVICE_HOSTNAME_STR,
      az_span_ptr(client._internal.iot_hub_hostname),
      _az_COUNTOF(TEST_DEVICE_HOSTNAME_STR) - 1);
}

static void test_az_iot_hub_client_init_custom_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  assert_memory_equal(
      TEST_DEVICE_ID_STR,
      az_span_ptr(client._internal.device_id),
      _az_COUNTOF(TEST_DEVICE_ID_STR) - 1);
  assert_memory_equal(
      TEST_DEVICE_HOSTNAME_STR,
      az_span_ptr(client._internal.iot_hub_hostname),
      _az_COUNTOF(TEST_DEVICE_HOSTNAME_STR) - 1);
  assert_memory_equal(
      TEST_MODULE_ID,
      az_span_ptr(client._internal.options.module_id),
      _az_COUNTOF(TEST_MODULE_ID) - 1);
  assert_memory_equal(
      TEST_USER_AGENT,
      az_span_ptr(client._internal.options.user_agent),
      _az_COUNTOF(TEST_USER_AGENT) - 1);
}

static void test_az_iot_hub_client_user_name_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_user_name,
      _az_COUNTOF(test_correct_user_name) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_user_name_get_twice_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  memset(test_span_buf, 0xFF, _az_COUNTOF(test_span_buf));
  az_span test_span = az_span_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));
  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);

  assert_memory_equal(
      test_correct_user_name, az_span_ptr(test_span), sizeof(test_correct_user_name) - 1);
  assert_int_equal(az_span_length(test_span), _az_COUNTOF(test_correct_user_name) - 1);
  assert_int_equal(az_span_capacity(test_span), TEST_SPAN_BUFFER_SIZE);
  assert_int_equal(test_span_buf[az_span_length(test_span)], 0xFF);

  assert_memory_equal(
      test_correct_user_name, az_span_ptr(test_span), sizeof(test_correct_user_name) - 1);
  assert_int_equal(az_span_length(test_span), _az_COUNTOF(test_correct_user_name) - 1);
  assert_int_equal(az_span_capacity(test_span), TEST_SPAN_BUFFER_SIZE);
  assert_int_equal(test_span_buf[az_span_length(test_span)], 0xFF);
}

static void test_az_iot_hub_client_user_name_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[sizeof(test_correct_user_name) - 2];
  az_span test_span = az_span_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));
  assert_int_equal(
      az_iot_hub_client_user_name_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(test_span), 0);
  assert_int_equal(az_span_capacity(test_span), sizeof(test_correct_user_name) - 2);
}

static void test_az_iot_hub_client_user_name_get_user_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_user_name_with_module_id,
      _az_COUNTOF(test_correct_user_name_with_module_id) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_user_name_get_user_options_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buf[sizeof(test_correct_user_name_with_module_id) - 2];
  az_span test_span = az_span_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));
  assert_int_equal(
      az_iot_hub_client_user_name_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(test_span), 0);
  assert_int_equal(az_span_capacity(test_span), sizeof(test_correct_user_name_with_module_id) - 2);
}

static void test_az_iot_hub_client_id_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_client_id,
      _az_COUNTOF(test_correct_client_id) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_id_get_twice_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  memset(test_span_buf, 0xFF, _az_COUNTOF(test_span_buf));
  az_span test_span = az_span_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));
  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);

  assert_memory_equal(
      test_correct_client_id, az_span_ptr(test_span), sizeof(test_correct_client_id) - 1);
  assert_int_equal(az_span_length(test_span), sizeof(test_correct_client_id) - 1);
  assert_int_equal(az_span_capacity(test_span), TEST_SPAN_BUFFER_SIZE);
  assert_int_equal(test_span_buf[az_span_length(test_span)], 0xFF);

  assert_memory_equal(
      test_correct_client_id, az_span_ptr(test_span), sizeof(test_correct_client_id) - 1);
  assert_int_equal(az_span_length(test_span), sizeof(test_correct_client_id) - 1);
  assert_int_equal(az_span_capacity(test_span), TEST_SPAN_BUFFER_SIZE);
  assert_int_equal(test_span_buf[az_span_length(test_span)], 0xFF);
}

static void test_az_iot_hub_client_id_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t test_span_buf[sizeof(test_correct_client_id) - 2];
  az_span test_span = az_span_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));
  assert_int_equal(
      az_iot_hub_client_id_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(test_span), 0);
  assert_int_equal(az_span_capacity(test_span), sizeof(test_correct_client_id) - 2);
}

static void test_az_iot_hub_client_id_get_module_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));

  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);
  az_span_for_test_verify(
      test_span,
      test_correct_client_id_with_module_id,
      _az_COUNTOF(test_correct_client_id_with_module_id) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_id_get_module_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buf[sizeof(test_correct_client_id_with_module_id) - 2];
  az_span test_span = az_span_init(test_span_buf, 0, _az_COUNTOF(test_span_buf));
  assert_int_equal(
      az_iot_hub_client_id_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(test_span), 0);
  assert_int_equal(az_span_capacity(test_span), sizeof(test_correct_client_id_with_module_id) - 2);
}

static void test_az_iot_hub_client_properties_init_succeed(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE] = { 0 };
  az_span test_span = az_span_init(test_span_buf, 0, sizeof(test_span_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_ptr_equal(props._internal.current_property, &test_span_buf[0]);
}

static void test_az_iot_hub_client_properties_init_user_set_params_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_ONE);
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);

  assert_memory_equal(
      az_span_ptr(props._internal.properties), test_correct_one_key_value, sizeof(test_correct_one_key_value) - 1);
}

static void test_az_iot_hub_client_properties_append_succeed(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, sizeof(test_span_buf));

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);

  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  az_span_for_test_verify(
      props._internal.properties,
      test_correct_one_key_value,
      _az_COUNTOF(test_correct_one_key_value) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_properties_append_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t test_span_buf[sizeof(test_correct_one_key_value) - 2];
  az_span test_span = az_span_init(test_span_buf, 0, sizeof(test_span_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_capacity(props._internal.properties), sizeof(test_correct_one_key_value) - 2);
  assert_int_equal(az_span_length(props._internal.properties), 0);
}

static void test_az_iot_hub_client_properties_append_twice_succeed(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, 0, sizeof(test_span_buf));

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);

  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_two, test_value_two), AZ_OK);
  az_span_for_test_verify(
      props._internal.properties,
      test_correct_two_key_value,
      _az_COUNTOF(test_correct_two_key_value) - 1,
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_properties_append_twice_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t test_span_buf[sizeof(test_correct_two_key_value) - 2];
  az_span test_span = az_span_init(test_span_buf, 0, sizeof(test_span_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_two, test_value_two),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(
      az_span_length(props._internal.properties), sizeof(test_correct_one_key_value) - 1);
  assert_int_equal(az_span_capacity(props._internal.properties), sizeof(test_correct_two_key_value) - 2);
}

int test_iot_hub_client()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_init_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_init_NULL_device_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_init_NULL_device_hostname_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_NULL_input_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_NULL_output_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_id_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_id_get_NULL_input_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_id_get_NULL_output_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_NULL_props_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_NULL_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_get_NULL_props_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_NULL_name_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_NULL_value_span_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_custom_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_id_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_id_get_module_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_module_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_user_set_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_twice_small_buffer_fail),
  };
  return cmocka_run_group_tests_name("az_iot_hub_client", tests, NULL, NULL);
}
