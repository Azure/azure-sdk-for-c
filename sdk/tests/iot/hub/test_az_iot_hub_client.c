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
#define TEST_HUB_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_DEVICE_ID_STR "my_device"
#define TEST_MODULE_ID "my_module_id"
#define TEST_API_VERSION "?api-version=2020-09-30"
#define TEST_USER_AGENT "os=azrtos"
#define TEST_MODEL_ID "dtmi:YOUR_COMPANY_NAME_HERE:sample_device;1"
#define TEST_PLATFORM_USER_AGENT "DeviceClientType=c%2F" AZ_SDK_VERSION_STRING
#define MODEL_ID_PARAMETER "model-id=dtmi%3AYOUR_COMPANY_NAME_HERE%3Asample_device%3B1"
#define CONTENT_TYPE_CBOR_PARAMETER \
  "default-content-type=" AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR

static const az_span test_hub_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_HUB_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);

static const char test_correct_user_name[] = TEST_HUB_HOSTNAME_STR
    "/" TEST_DEVICE_ID_STR "/" TEST_API_VERSION "&" TEST_PLATFORM_USER_AGENT;

static const char test_correct_user_name_with_module_id_user_agent_options[] = TEST_HUB_HOSTNAME_STR
    "/" TEST_DEVICE_ID_STR "/" TEST_MODULE_ID "/" TEST_API_VERSION "&" TEST_USER_AGENT;
static const char test_correct_user_name_with_content_type_option[]
    = TEST_HUB_HOSTNAME_STR "/" TEST_DEVICE_ID_STR "/" TEST_API_VERSION "&" TEST_PLATFORM_USER_AGENT
                            "&" CONTENT_TYPE_CBOR_PARAMETER;
static const char test_correct_user_name_with_model_id_option[] = TEST_HUB_HOSTNAME_STR
    "/" TEST_DEVICE_ID_STR "/" TEST_API_VERSION "&" TEST_PLATFORM_USER_AGENT "&" MODEL_ID_PARAMETER;

static const char test_correct_user_name_with_module_id_user_agent_content_type_options[]
    = TEST_HUB_HOSTNAME_STR "/" TEST_DEVICE_ID_STR "/" TEST_MODULE_ID "/" TEST_API_VERSION
                            "&" TEST_USER_AGENT "&" CONTENT_TYPE_CBOR_PARAMETER;
static const char test_correct_user_name_with_module_id_user_agent_model_id_options[]
    = TEST_HUB_HOSTNAME_STR "/" TEST_DEVICE_ID_STR "/" TEST_MODULE_ID "/" TEST_API_VERSION
                            "&" TEST_USER_AGENT "&" MODEL_ID_PARAMETER;
static const char test_correct_user_name_with_content_type_model_id_options[]
    = TEST_HUB_HOSTNAME_STR "/" TEST_DEVICE_ID_STR "/" TEST_API_VERSION "&" TEST_PLATFORM_USER_AGENT
                            "&" CONTENT_TYPE_CBOR_PARAMETER "&" MODEL_ID_PARAMETER;

static const char test_correct_user_name_with_module_id_user_agent_content_type_model_id_options[]
    = TEST_HUB_HOSTNAME_STR "/" TEST_DEVICE_ID_STR "/" TEST_MODULE_ID "/" TEST_API_VERSION
                            "&" TEST_USER_AGENT "&" CONTENT_TYPE_CBOR_PARAMETER
                            "&" MODEL_ID_PARAMETER;

static const char test_correct_client_id[] = TEST_DEVICE_ID_STR;
static const char test_correct_client_id_with_module_id[] = TEST_DEVICE_ID_STR "/" TEST_MODULE_ID;

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_iot_hub_client_init_NULL_client_fails(void** state)
{
  (void)state;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_init(NULL, test_hub_hostname, test_device_id, NULL));
}

static void test_az_iot_hub_client_init_NULL_device_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_init(&client, test_hub_hostname, AZ_SPAN_EMPTY, NULL));
}

static void test_az_iot_hub_client_init_NULL_hub_hostname_id_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_init(&client, AZ_SPAN_EMPTY, test_device_id, NULL));
}

static void test_az_iot_hub_client_get_user_name_NULL_client_fails(void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_get_user_name(NULL, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_get_user_name_NULL_char_buffer_fails(void** state)
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

static void test_az_iot_hub_client_get_client_id_NULL_char_buffer_fails(void** state)
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

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_iot_hub_client_get_default_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_span_is_content_equal(options.module_id, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(
      options.user_agent, az_span_create_from_str(TEST_PLATFORM_USER_AGENT)));
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
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);
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
  assert_memory_equal(
      AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR,
      az_span_ptr(client._internal.options.method_twin_content_type),
      _az_COUNTOF(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR) - 1);
  assert_memory_equal(
      TEST_MODEL_ID,
      az_span_ptr(client._internal.options.model_id),
      _az_COUNTOF(TEST_MODEL_ID) - 1);
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
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_hub_client_get_user_name_with_module_id_user_agent_options_succeed(
    void** state)
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
  assert_string_equal(test_correct_user_name_with_module_id_user_agent_options, mqtt_topic_buf);
  assert_int_equal(
      sizeof(test_correct_user_name_with_module_id_user_agent_options) - 1, test_length);
}

static void
test_az_iot_hub_client_get_user_name_with_module_id_user_agent_options_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_module_id_user_agent_options) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_hub_client_get_user_name_with_content_type_option_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_correct_user_name_with_content_type_option, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_user_name_with_content_type_option) - 1, test_length);
}

static void test_az_iot_hub_client_get_user_name_with_content_type_option_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_content_type_option) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_option_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_correct_user_name_with_model_id_option, mqtt_topic_buf);
  assert_int_equal(sizeof(test_correct_user_name_with_model_id_option) - 1, test_length);
}

static void test_az_iot_hub_client_get_user_name_with_model_id_option_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_model_id_option) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void
test_az_iot_hub_client_get_user_name_with_model_id_option_small_buffer_first_if_case_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name) - 1];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void
test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_options_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);

  assert_string_equal(
      test_correct_user_name_with_module_id_user_agent_content_type_options, mqtt_topic_buf);
  assert_int_equal(
      sizeof(test_correct_user_name_with_module_id_user_agent_content_type_options) - 1,
      test_length);
}

static void
test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_options_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf
      [sizeof(test_correct_user_name_with_module_id_user_agent_content_type_options) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_hub_client_get_user_name_with_module_id_user_agent_model_id_options_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);
  assert_string_equal(
      test_correct_user_name_with_module_id_user_agent_model_id_options, mqtt_topic_buf);
  assert_int_equal(
      sizeof(test_correct_user_name_with_module_id_user_agent_model_id_options) - 1, test_length);
}

static void
test_az_iot_hub_client_get_user_name_with_module_id_user_agent_model_id_options_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char
      mqtt_topic_buf[sizeof(test_correct_user_name_with_module_id_user_agent_model_id_options) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_hub_client_get_user_name_with_content_type_model_id_options_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_correct_user_name_with_content_type_model_id_options, mqtt_topic_buf);
  assert_int_equal(
      sizeof(test_correct_user_name_with_content_type_model_id_options) - 1, test_length);
}

static void
test_az_iot_hub_client_get_user_name_with_content_type_model_id_options_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[sizeof(test_correct_user_name_with_content_type_model_id_options) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void
test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_model_id_options_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_OK);

  assert_string_equal(
      test_correct_user_name_with_module_id_user_agent_content_type_model_id_options,
      mqtt_topic_buf);
  assert_int_equal(
      sizeof(test_correct_user_name_with_module_id_user_agent_content_type_model_id_options) - 1,
      test_length);
}

static void
test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_model_id_options_small_buffer_fail(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.module_id = AZ_SPAN_FROM_STR(TEST_MODULE_ID);
  options.user_agent = AZ_SPAN_FROM_STR(TEST_USER_AGENT);
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);
  options.model_id = AZ_SPAN_FROM_STR(TEST_MODEL_ID);

  assert_int_equal(
      az_iot_hub_client_init(&client, test_hub_hostname, test_device_id, &options), AZ_OK);

  char mqtt_topic_buf
      [sizeof(test_correct_user_name_with_module_id_user_agent_content_type_model_id_options) - 2];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_get_user_name(
          &client, mqtt_topic_buf, sizeof(mqtt_topic_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
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
      AZ_ERROR_NOT_ENOUGH_SPACE);
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
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

int test_az_iot_hub_client()
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
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_NULL_char_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_NULL_output_span_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_NULL_char_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_NULL_output_span_fails),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_get_default_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_succeed),
    cmocka_unit_test(test_az_iot_hub_client_init_custom_options_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_small_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_options_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_with_content_type_option_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_content_type_option_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_with_model_id_option_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_user_name_with_model_id_option_small_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_model_id_option_small_buffer_first_if_case_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_options_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_options_small_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_model_id_options_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_model_id_options_small_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_content_type_model_id_options_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_content_type_model_id_options_small_buffer_fail),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_model_id_options_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_get_user_name_with_module_id_user_agent_content_type_model_id_options_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_small_buffer_fail),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_module_succeed),
    cmocka_unit_test(test_az_iot_hub_client_get_client_id_module_small_buffer_fail),
  };
  return cmocka_run_group_tests_name("az_iot_hub_client", tests, NULL, NULL);
}
