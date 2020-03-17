// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_hub_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

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

void test_az_iot_hub_client_get_default_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_span_is_content_equal(options.module_id, AZ_SPAN_NULL));
  assert_true(az_span_is_content_equal(options.user_agent, AZ_SPAN_NULL));
}

void test_az_iot_hub_client_init_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  assert_string_equal(TEST_DEVICE_ID_STR, az_span_ptr(client._internal.device_id));
  assert_string_equal(TEST_DEVICE_HOSTNAME_STR, az_span_ptr(client._internal.iot_hub_hostname));
}

void test_az_iot_hub_client_init_custom_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  assert_string_equal(TEST_DEVICE_ID_STR, az_span_ptr(client._internal.device_id));
  assert_string_equal(TEST_DEVICE_HOSTNAME_STR, az_span_ptr(client._internal.iot_hub_hostname));
  assert_string_equal(TEST_MODULE_ID, az_span_ptr(client._internal.options.module_id));
  assert_string_equal(TEST_USER_AGENT, az_span_ptr(client._internal.options.user_agent));
}

void test_az_iot_hub_client_user_name_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name) - 1];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);

  assert_memory_equal(
      test_correct_user_name, az_span_ptr(test_span), sizeof(test_correct_user_name) - 1);
}

void test_az_iot_hub_client_user_name_get_as_string_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name)] = { 0 };
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);

  assert_int_equal(az_span_length(test_span), sizeof(test_correct_user_name) - 1);
  assert_string_equal(test_correct_user_name, az_span_ptr(test_span));
}

void test_az_iot_hub_client_user_name_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name) - 2];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(
      az_iot_hub_client_user_name_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_user_name_get_user_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name_with_module_id) - 1];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);

  assert_memory_equal(
      test_correct_user_name_with_module_id,
      az_span_ptr(test_span),
      sizeof(test_correct_user_name_with_module_id) - 1);
}

void test_az_iot_hub_client_user_name_get_user_options_as_string_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name_with_module_id)] = { 0 };
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_user_name_get(&client, test_span, &test_span), AZ_OK);

  assert_int_equal(az_span_length(test_span), sizeof(test_correct_user_name_with_module_id) - 1);
  assert_string_equal(test_correct_user_name_with_module_id, az_span_ptr(test_span));
}

void test_az_iot_hub_client_user_name_get_user_options_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name_with_module_id) - 2];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(
      az_iot_hub_client_user_name_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_id_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id) - 1];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);

  assert_memory_equal(
      test_correct_client_id, az_span_ptr(test_span), sizeof(test_correct_client_id) - 1);
}

void test_az_iot_hub_client_id_get_as_string_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id)] = { 0 };
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);

  assert_int_equal(az_span_length(test_span), sizeof(test_correct_client_id) - 1);
  assert_string_equal(test_correct_client_id, az_span_ptr(test_span));
}

void test_az_iot_hub_client_id_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id) - 2];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(
      az_iot_hub_client_id_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_id_get_module_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id_with_module_id) - 1];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);

  assert_memory_equal(
      test_correct_client_id_with_module_id,
      az_span_ptr(test_span),
      sizeof(test_correct_client_id_with_module_id) - 1);
}

void test_az_iot_hub_client_id_get_module_as_string_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id_with_module_id)] = { 0 };
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(az_iot_hub_client_id_get(&client, test_span, &test_span), AZ_OK);

  assert_int_equal(az_span_length(test_span), sizeof(test_correct_client_id_with_module_id) - 1);
  assert_string_equal(test_correct_client_id_with_module_id, az_span_ptr(test_span));
}

void test_az_iot_hub_client_id_get_module_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options), AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id_with_module_id) - 2];
  az_span test_span = az_span_init(test_span_buffer, 0, _az_COUNTOF(test_span_buffer));
  assert_int_equal(
      az_iot_hub_client_id_get(&client, test_span, &test_span),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_properties_init_succeed(void** state)
{
  (void)state;

  uint8_t test_buf[10] = { 0 };
  az_span test_span = az_span_init(test_buf, 0, sizeof(test_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_ptr_equal(props._internal.current_property, &test_buf[0]);
}

void test_az_iot_hub_client_properties_init_user_set_params_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_ONE);
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);

  assert_memory_equal(
      az_span_ptr(test_span), test_correct_one_key_value, sizeof(test_correct_one_key_value) - 1);
}

void test_az_iot_hub_client_properties_append_succeed(void** state)
{
  (void)state;

  uint8_t test_buf[sizeof(test_correct_one_key_value)];
  az_span test_span = az_span_init(test_buf, 0, sizeof(test_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_memory_equal(
      test_correct_one_key_value, az_span_ptr(test_span), sizeof(test_correct_one_key_value) - 1);
}

void test_az_iot_hub_client_properties_append_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t test_buf[sizeof(test_correct_one_key_value) - 2];
  az_span test_span = az_span_init(test_buf, 0, sizeof(test_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(az_span_length(props._internal.properties), 0);
}

void test_az_iot_hub_client_properties_append_twice_succeed(void** state)
{
  (void)state;

  uint8_t test_buf[sizeof(test_correct_two_key_value)];
  az_span test_span = az_span_init(test_buf, 0, sizeof(test_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_two, test_value_two), AZ_OK);
  assert_memory_equal(
      test_correct_two_key_value, az_span_ptr(test_span), sizeof(test_correct_two_key_value) - 1);
}

void test_az_iot_hub_client_properties_append_twice_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t test_buf[sizeof(test_correct_two_key_value) - 2];
  az_span test_span = az_span_init(test_buf, 0, sizeof(test_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_two, test_value_two),
      AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
  assert_int_equal(
      az_span_length(props._internal.properties), sizeof(test_correct_one_key_value) - 1);
}

int test_iot_hub_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_hub_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_custom_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_as_string_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_as_string_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_id_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_as_string_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_id_get_module_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_module_as_string_succeed),
    cmocka_unit_test(test_az_iot_hub_client_id_get_module_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_user_set_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_twice_small_buffer_fail),
  };
  return cmocka_run_group_tests_name("az_iot_client", tests, NULL, NULL);
}
