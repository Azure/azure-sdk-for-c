// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_test_span.h>
#include <azure/core/az_span.h>
#include <azure/core/az_version.h>
#include <azure/iot/az_iot_hub_client.h>

#include <azure/core/az_precondition.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 256

// Hub Client
#define TEST_DEVICE_ID_STR "my_device"
#define TEST_HUB_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_MODULE_ID "my_module_id"
#define TEST_USER_AGENT "os=azrtos"
#define TEST_MODEL_ID "dtmi:YOUR_COMPANY_NAME_HERE:sample_device;1"
#define PLATFORM_USER_AGENT "DeviceClientType=c%2F" AZ_SDK_VERSION_STRING

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static const az_span test_hub_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_HUB_HOSTNAME_STR);

static const char test_correct_user_name[]
    = "myiothub.azure-devices.net/my_device/?api-version=2018-06-30&" PLATFORM_USER_AGENT;
static const char test_correct_user_name_with_model_id[]
    = "myiothub.azure-devices.net/my_device/?api-version=2020-05-31-preview&" PLATFORM_USER_AGENT
      "&model-id=dtmi%3AYOUR_COMPANY_NAME_HERE%3Asample_device%3B1";
static const char test_correct_user_name_with_model_id_with_module_id[]
    = "myiothub.azure-devices.net/my_device/my_module_id/"
      "?api-version=2020-05-31-preview&os=azrtos&model-id=dtmi%3AYOUR_COMPANY_NAME_HERE%"
      "3Asample_device%3B1";
static const char test_correct_user_name_with_module_id[]
    = "myiothub.azure-devices.net/my_device/my_module_id/?api-version=2018-06-30&os=azrtos";
static const char test_correct_client_id[] = "my_device";
static const char test_correct_client_id_with_module_id[] = "my_device/my_module_id";

// Properties
#define TEST_KEY "key"
#define TEST_KEY_ONE "key_one"
#define TEST_KEY_TWO "key_two"
#define TEST_KEY_THREE "key_three"
#define TEST_VALUE_ONE "value_one"
#define TEST_VALUE_TWO "value_two"
#define TEST_VALUE_THREE "value_three"
#define TEST_KEY_VALUE_ONE "key_one=value_one"
#define TEST_KEY_VALUE_TWO "key_one=value_one&key_two=value_two"
#define TEST_KEY_VALUE_SUBSTRING "key_one=value_one&key=value_two"
#define TEST_KEY_VALUE_SAME "key_one=key&key=value_two"
#define TEST_KEY_VALUE_THREE "key_one=value_one&key_two=value_two&key_three=value_three"

static const az_span test_key = AZ_SPAN_LITERAL_FROM_STR(TEST_KEY);
static const az_span test_key_one = AZ_SPAN_LITERAL_FROM_STR(TEST_KEY_ONE);
static const az_span test_key_two = AZ_SPAN_LITERAL_FROM_STR(TEST_KEY_TWO);
static const az_span test_key_three = AZ_SPAN_LITERAL_FROM_STR(TEST_KEY_THREE);
static const az_span test_value_one = AZ_SPAN_LITERAL_FROM_STR(TEST_VALUE_ONE);
static const az_span test_value_two = AZ_SPAN_LITERAL_FROM_STR(TEST_VALUE_TWO);
static const az_span test_value_three = AZ_SPAN_LITERAL_FROM_STR(TEST_VALUE_THREE);

static const char test_correct_one_key_value[] = "key_one=value_one";
static const char test_correct_two_key_value[] = "key_one=value_one&key_two=value_two";

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_iot_hub_client_init_NULL_client_fails(void** state)
{
  (void)state;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_init(NULL, test_device_id, test_hub_hostname, NULL));
}

static void test_az_iot_hub_client_init_NULL_device_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_init(&client, AZ_SPAN_NULL, test_hub_hostname, NULL));
}

static void test_az_iot_hub_client_init_NULL_hub_hostname_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_init(&client, test_device_id, AZ_SPAN_NULL, NULL));
}

static void test_az_iot_hub_client_get_user_name_NULL_client_fails(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_get_user_name(NULL, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_get_user_name_NULL_input_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_get_user_name(&client, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_get_user_name_NULL_output_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_get_user_name(&client, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_get_client_id_NULL_client_fails(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_get_client_id(NULL, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_get_client_id_NULL_input_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_get_client_id(&client, NULL, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_get_client_id_NULL_output_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_get_client_id(&client, test_buf, 0, &test_length));
}

static void test_az_iot_hub_client_properties_init_NULL_props_fails(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, _az_COUNTOF(test_span_buf));

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_properties_init(NULL, test_span, 0));
}

static void test_az_iot_hub_client_properties_append_get_NULL_props_fails(void** state)
{
  (void)state;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_properties_append(NULL, test_key_one, test_value_one));
}

static void test_az_iot_hub_client_properties_append_NULL_name_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_properties_append(&props, AZ_SPAN_NULL, test_value_one));
}

static void test_az_iot_hub_client_properties_append_NULL_value_span_fails(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_properties_append(&props, test_key_one, AZ_SPAN_NULL));
}

static void test_az_iot_hub_client_properties_find_NULL_props_fail(void** state)
{
  (void)state;

  az_span out_value;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_properties_find(NULL, test_key_one, &out_value));
}

static void test_az_iot_hub_client_properties_find_NULL_name_fail(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  az_span out_value;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_properties_find(&props, AZ_SPAN_NULL, &out_value));
}

static void test_az_iot_hub_client_properties_find_NULL_value_fail(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_properties_find(&props, test_key_one, NULL));
}

static void test_az_iot_hub_client_properties_next_NULL_props_fail(void** state)
{
  (void)state;

  az_pair pair_out;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_properties_next(NULL, &pair_out));
}

static void test_az_iot_hub_client_properties_next_NULL_out_fail(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_properties_next(&props, NULL));
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_get_default_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_span_is_content_equal(options.module_id, AZ_SPAN_NULL));
  assert_true(az_span_is_content_equal(options.user_agent, az_span_from_str(PLATFORM_USER_AGENT)));
}

static void test_az_iot_hub_client_init_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, NULL), AZ_OK);

  assert_memory_equal(
      TEST_DEVICE_ID_STR,
      az_span_ptr(client._internal.device_id),
      _az_COUNTOF(TEST_DEVICE_ID_STR) - 1);
  assert_memory_equal(
      TEST_HUB_HOSTNAME_STR,
      az_span_ptr(client._internal.iot_hub_hostname),
      _az_COUNTOF(TEST_HUB_HOSTNAME_STR) - 1);
}

static void test_az_iot_hub_client_init_custom_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  assert_memory_equal(
      TEST_DEVICE_ID_STR,
      az_span_ptr(client._internal.device_id),
      _az_COUNTOF(TEST_DEVICE_ID_STR) - 1);
  assert_memory_equal(
      TEST_HUB_HOSTNAME_STR,
      az_span_ptr(client._internal.iot_hub_hostname),
      _az_COUNTOF(TEST_HUB_HOSTNAME_STR) - 1);
  assert_memory_equal(
      TEST_MODULE_ID,
      az_span_ptr(client._internal.options.module_id),
      _az_COUNTOF(TEST_MODULE_ID) - 1);
  assert_memory_equal(
      TEST_USER_AGENT,
      az_span_ptr(client._internal.options.user_agent),
      _az_COUNTOF(TEST_USER_AGENT) - 1);
}

static void test_az_iot_hub_client_get_user_name_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, NULL), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_correct_user_name, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_user_name) - 1, test_length);
}

static void test_az_iot_hub_client_get_user_name_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, NULL), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_get_user_name_user_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_user_name_with_module_id, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_user_name_with_module_id) - 1, test_length);
}

static void test_az_iot_hub_client_get_user_name_user_options_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_module_id) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &test_length),
      AZ_OK);

  assert_string_equal(test_correct_user_name_with_model_id, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_user_name_with_model_id) - 1, test_length);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_model_id) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client,
          mqtt_topic_buf,
          sizeof(mqtt_topic_buf),
          &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_small_buffer_first_if_case_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name) + 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_user_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_user_name_with_model_id_with_module_id, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_user_name_with_model_id_with_module_id) - 1, test_length);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_user_options_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_model_id_with_module_id) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_get_client_id_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, NULL), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_client_id(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_client_id, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_client_id) - 1, test_length);
}

static void test_az_iot_hub_client_get_client_id_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, NULL), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_client_id) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_client_id(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_get_client_id_module_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_client_id(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);
  assert_string_equal(test_correct_client_id_with_module_id, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_client_id_with_module_id) - 1, test_length);
}

static void test_az_iot_hub_client_get_client_id_module_small_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_client_id_with_module_id) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_client_id(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_properties_init_succeed(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE] = { 0 };
  az_span test_span = az_span_init(test_span_buf, sizeof(test_span_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span, 0), AZ_OK);
  assert_int_equal(props._internal.current_property_index, 0);
}

static void test_az_iot_hub_client_properties_init_user_set_params_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_ONE);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  assert_memory_equal(
      az_span_ptr(props._internal.properties_buffer),
      test_correct_one_key_value,
      sizeof(test_correct_one_key_value) - 1);
}

static void test_az_iot_hub_client_properties_append_succeed(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, sizeof(test_span_buf));

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span, 0), AZ_OK);

  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  az_span_for_test_verify(
      az_span_slice(props._internal.properties_buffer, 0, props._internal.properties_written),
      test_correct_one_key_value,
      _az_COUNTOF(test_correct_one_key_value) - 1,
      az_span_init(test_span_buf, _az_COUNTOF(test_span_buf)),
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_properties_append_empty_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, AZ_SPAN_NULL, 0), AZ_OK);

  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void test_az_iot_hub_client_properties_append_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t test_span_buf[sizeof(test_correct_one_key_value) - 2];
  az_span test_span = az_span_init(test_span_buf, sizeof(test_span_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span, 0), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
  assert_int_equal(
      az_span_size(props._internal.properties_buffer), sizeof(test_correct_one_key_value) - 2);
}

static void test_az_iot_hub_client_properties_append_twice_succeed(void** state)
{
  (void)state;

  uint8_t test_span_buf[TEST_SPAN_BUFFER_SIZE];
  az_span test_span = az_span_for_test_init(test_span_buf, sizeof(test_span_buf));

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span, 0), AZ_OK);

  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_two, test_value_two), AZ_OK);
  az_span_for_test_verify(
      az_span_slice(props._internal.properties_buffer, 0, props._internal.properties_written),
      test_correct_two_key_value,
      _az_COUNTOF(test_correct_two_key_value) - 1,
      az_span_init(test_span_buf, _az_COUNTOF(test_span_buf)),
      TEST_SPAN_BUFFER_SIZE);
}

static void test_az_iot_hub_client_properties_append_twice_small_buffer_fail(void** state)
{
  (void)state;

  uint8_t test_span_buf[sizeof(test_correct_two_key_value) - 2];
  az_span test_span = az_span_init(test_span_buf, sizeof(test_span_buf));
  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, test_span, 0), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_one, test_value_one), AZ_OK);
  assert_int_equal(
      az_iot_hub_client_properties_append(&props, test_key_two, test_value_two),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
  assert_int_equal(props._internal.properties_written, sizeof(test_correct_one_key_value) - 1);
  assert_int_equal(
      az_span_size(props._internal.properties_buffer), sizeof(test_correct_two_key_value) - 2);
}

static void test_az_iot_hub_client_properties_find_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_ONE);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(az_iot_hub_client_properties_find(&props, test_key_one, &out_value), AZ_OK);
  assert_memory_equal(
      az_span_ptr(out_value), az_span_ptr(test_value_one), (size_t)az_span_size(test_value_one));
}

static void test_az_iot_hub_client_properties_find_middle_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_THREE);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(az_iot_hub_client_properties_find(&props, test_key_two, &out_value), AZ_OK);
  assert_memory_equal(
      az_span_ptr(out_value), az_span_ptr(test_value_two), (size_t)az_span_size(test_value_two));
}

static void test_az_iot_hub_client_properties_find_end_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_TWO);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(az_iot_hub_client_properties_find(&props, test_key_two, &out_value), AZ_OK);
  assert_memory_equal(
      az_span_ptr(out_value), az_span_ptr(test_value_two), (size_t)az_span_size(test_value_two));
}

static void test_az_iot_hub_client_properties_find_substring_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_SUBSTRING);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(az_iot_hub_client_properties_find(&props, test_key, &out_value), AZ_OK);
  assert_memory_equal(
      az_span_ptr(out_value), az_span_ptr(test_value_two), (size_t)az_span_size(test_value_two));
}

static void test_az_iot_hub_client_properties_find_name_value_same_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_SAME);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(az_iot_hub_client_properties_find(&props, test_key, &out_value), AZ_OK);
  assert_memory_equal(
      az_span_ptr(out_value), az_span_ptr(test_value_two), (size_t)az_span_size(test_value_two));
}

static void test_az_iot_hub_client_properties_find_empty_buffer_fail(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;

  assert_int_equal(az_iot_hub_client_properties_init(&props, AZ_SPAN_NULL, 0), AZ_OK);

  az_span out_value;
  assert_int_equal(
      az_iot_hub_client_properties_find(&props, test_key_one, &out_value), AZ_ERROR_ITEM_NOT_FOUND);
}

static void test_az_iot_hub_client_properties_find_fail(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_ONE);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(
      az_iot_hub_client_properties_find(&props, AZ_SPAN_FROM_STR("key_foo"), &out_value),
      AZ_ERROR_ITEM_NOT_FOUND);
}

static void test_az_iot_hub_client_properties_find_substring_fail(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_TWO);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(
      az_iot_hub_client_properties_find(&props, AZ_SPAN_FROM_STR("key"), &out_value),
      AZ_ERROR_ITEM_NOT_FOUND);
}

static void test_az_iot_hub_client_properties_find_substring_suffix_fail(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_TWO);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(
      az_iot_hub_client_properties_find(&props, AZ_SPAN_FROM_STR("one"), &out_value),
      AZ_ERROR_ITEM_NOT_FOUND);
}

static void test_az_iot_hub_client_properties_find_value_match_fail(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_THREE);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(
      az_iot_hub_client_properties_find(&props, AZ_SPAN_FROM_STR("value_two"), &out_value),
      AZ_ERROR_ITEM_NOT_FOUND);
}

static void test_az_iot_hub_client_properties_find_value_match_end_fail(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_THREE);
  az_iot_hub_client_properties props;

  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_span out_value;
  assert_int_equal(
      az_iot_hub_client_properties_find(&props, AZ_SPAN_FROM_STR("value_three"), &out_value),
      AZ_ERROR_ITEM_NOT_FOUND);
}

static void test_az_iot_hub_client_properties_next_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_THREE);
  az_iot_hub_client_properties props;
  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_pair pair_out;

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_one), (size_t)az_span_size(test_key_one));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_one),
      (size_t)az_span_size(test_value_one));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_two), (size_t)az_span_size(test_key_two));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_two),
      (size_t)az_span_size(test_value_two));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_three), (size_t)az_span_size(test_key_three));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_three),
      (size_t)az_span_size(test_value_three));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_ERROR_EOF);
}

static void test_az_iot_hub_client_properties_next_twice_succeed(void** state)
{
  (void)state;

  az_span test_span = az_span_from_str(TEST_KEY_VALUE_TWO);
  az_iot_hub_client_properties props;
  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  az_pair pair_out;

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_one), (size_t)az_span_size(test_key_one));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_one),
      (size_t)az_span_size(test_value_one));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_two), (size_t)az_span_size(test_key_two));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_two),
      (size_t)az_span_size(test_value_two));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_ERROR_EOF);

  // Reset to beginning of span
  assert_int_equal(
      az_iot_hub_client_properties_init(&props, test_span, az_span_size(test_span)), AZ_OK);

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_one), (size_t)az_span_size(test_key_one));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_one),
      (size_t)az_span_size(test_value_one));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_OK);
  assert_memory_equal(
      az_span_ptr(pair_out.key), az_span_ptr(test_key_two), (size_t)az_span_size(test_key_two));
  assert_memory_equal(
      az_span_ptr(pair_out.value),
      az_span_ptr(test_value_two),
      (size_t)az_span_size(test_value_two));

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_ERROR_EOF);
}

static void test_az_iot_hub_client_properties_next_empty_succeed(void** state)
{
  (void)state;

  az_iot_hub_client_properties props;
  assert_int_equal(az_iot_hub_client_properties_init(&props, AZ_SPAN_NULL, 0), AZ_OK);

  az_pair pair_out;

  assert_int_equal(az_iot_hub_client_properties_next(&props, &pair_out), AZ_ERROR_EOF);
}

int test_iot_hub_client()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_init_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_init_NULL_device_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_init_NULL_hub_hostname_id_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_NULL_input_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_NULL_output_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_NULL_input_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_NULL_output_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_NULL_props_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_get_NULL_props_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_NULL_name_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_NULL_value_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_NULL_props_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_NULL_name_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_NULL_value_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_next_NULL_props_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_next_NULL_out_fail),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_custom_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_user_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_user_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_with_model_id_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_with_model_id_small_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_model_id_small_buffer_first_if_case_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_with_model_id_user_options_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_model_id_user_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_module_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_module_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_init_user_set_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_empty_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_append_twice_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_middle_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_end_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_substring_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_name_value_same_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_empty_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_substring_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_substring_suffix_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_value_match_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_find_value_match_end_fail),
    cmocka_unit_test(test_az_iot_hub_client_properties_next_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_next_twice_succeed),
    cmocka_unit_test(test_az_iot_hub_client_properties_next_empty_succeed),
  };
  return cmocka_run_group_tests_name("az_iot_hub_client", tests, NULL, NULL);
}
