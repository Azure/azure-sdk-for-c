// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_hub_client.h>
#include <az_span.h>

#include <stdio.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

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
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

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
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

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
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name)];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(az_iot_hub_client_user_name_get(&client, test_span, &test_span) == AZ_OK);

  assert_string_equal(test_correct_user_name, az_span_ptr(test_span));
}

void test_az_iot_hub_client_user_name_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name) - 1];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(
      az_iot_hub_client_user_name_get(&client, test_span, &test_span)
      == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_user_name_get_user_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name_with_module_id)];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(az_iot_hub_client_user_name_get(&client, test_span, &test_span) == AZ_OK);

  assert_string_equal(test_correct_user_name_with_module_id, az_span_ptr(test_span));
}

void test_az_iot_hub_client_user_name_get_user_options_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_user_name_with_module_id) - 1];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(
      az_iot_hub_client_user_name_get(&client, test_span, &test_span)
      == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_client_id_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id)];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(az_iot_hub_client_id_get(&client, test_span, &test_span) == AZ_OK);

  assert_string_equal(test_correct_client_id, az_span_ptr(test_span));
}

void test_az_iot_hub_client_client_id_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correct_client_id) - 1];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(
      az_iot_hub_client_id_get(&client, test_span, &test_span)
      == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

void test_az_iot_hub_client_client_id_module_get_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correcet_client_id_with_module_id)];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(az_iot_hub_client_id_get(&client, test_span, &test_span) == AZ_OK);

  assert_string_equal(test_correct_client_id_with_module_id, az_span_ptr(test_span));
}

void test_az_iot_hub_client_client_id_module_get_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t test_span_buffer[sizeof(test_correcet_client_id_with_module_id) - 1];
  az_span test_span
      = az_span_init(test_span_buffer, 0, sizeof(test_span_buffer) / sizeof(test_span_buffer[0]));
  assert_true(
      az_iot_hub_client_id_get(&client, test_span, &test_span)
      == AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY);
}

int test_iot_hub_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_hub_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_custom_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_user_name_get_user_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_client_id_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_client_id_get_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_client_id_module_get_succeed),
    cmocka_unit_test(test_az_iot_hub_client_client_id_module_get_small_buffer_fail),
  };
  return cmocka_run_group_tests_name("az_iot_client", tests, NULL, NULL);
}
