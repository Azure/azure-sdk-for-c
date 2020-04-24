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

#define TEST_SPAN_BUFFER_SIZE 256

#define TEST_DEVICE_ID_STR "my_device"
#define TEST_MODULE_ID_STR "my_module"
#define TEST_DEVICE_HOSTNAME_STR "myiothub.azure-devices.net"
#define TEST_SIG "cS1eHM%2FlDjsRsrZV9508wOFrgmZk4g8FNg8NwHVSiSQ"
#define TEST_EXPIRATION_STR "1578941692"
#define TEST_KEY_NAME "iothubowner"

static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_HOSTNAME_STR);
static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR(TEST_DEVICE_ID_STR);
static const az_span test_module_id = AZ_SPAN_LITERAL_FROM_STR(TEST_MODULE_ID_STR);
static const uint32_t test_sas_expiry_time_secs = 1578941692;
static const az_span test_signature = AZ_SPAN_LITERAL_FROM_STR(TEST_SIG);

#ifndef NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void az_iot_hub_client_sas_get_signature_NULL_signature_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span signature = AZ_SPAN_NULL;

  assert_precondition_checked(
      az_iot_hub_client_sas_get_signature(&client, test_sas_expiry_time_secs, signature, NULL));
}

static void az_iot_hub_client_sas_get_signature_NULL_signature_span_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span signature = AZ_SPAN_NULL;

  assert_precondition_checked(az_iot_hub_client_sas_get_signature(
      &client, test_sas_expiry_time_secs, signature, &signature));
}

static void az_iot_hub_client_sas_get_signature_NULL_client_fails(void** state)
{
  (void)state;
  uint8_t signature_buffer[TEST_SPAN_BUFFER_SIZE];
  az_span signature = az_span_init(signature_buffer, _az_COUNTOF(signature_buffer));

  assert_precondition_checked(
      az_iot_hub_client_sas_get_signature(NULL, test_sas_expiry_time_secs, signature, &signature));
}

static void az_iot_hub_client_sas_get_password_EMPTY_signature_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_precondition_checked(az_iot_hub_client_sas_get_password(
      &client, signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length));
}

static void az_iot_hub_client_sas_get_password_NULL_password_span_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span key_name = AZ_SPAN_NULL;
  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_precondition_checked(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, NULL, _az_COUNTOF(password), &length));
}

static void az_iot_hub_client_sas_get_password_empty_password_buffer_span_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span key_name = AZ_SPAN_NULL;
  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_precondition_checked(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, 0, &length));
}

#endif // NO_PRECONDITION_CHECKING

static void az_iot_hub_client_sas_get_signature_device_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  const char* expected_signature
      = TEST_DEVICE_HOSTNAME_STR "/devices/" TEST_DEVICE_ID_STR "\n" TEST_EXPIRATION_STR;

  uint8_t signature_buffer[TEST_SPAN_BUFFER_SIZE];
  az_span signature = az_span_for_test_init(signature_buffer, _az_COUNTOF(signature_buffer));
  az_span out_signature;

  assert_true(az_succeeded(az_iot_hub_client_sas_get_signature(
      &client, test_sas_expiry_time_secs, signature, &out_signature)));

  az_span_for_test_verify(
      out_signature, expected_signature, (int32_t)strlen(expected_signature), signature, TEST_SPAN_BUFFER_SIZE);
}

static void az_iot_hub_client_sas_get_signature_module_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = test_module_id;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  const char* expected_signature = TEST_DEVICE_HOSTNAME_STR
      "/devices/" TEST_DEVICE_ID_STR "/modules/" TEST_MODULE_ID_STR "\n" TEST_EXPIRATION_STR;

  uint8_t signature_buffer[TEST_SPAN_BUFFER_SIZE];
  az_span signature = az_span_for_test_init(signature_buffer, _az_COUNTOF(signature_buffer));
  az_span out_signature;

  assert_true(az_succeeded(az_iot_hub_client_sas_get_signature(
      &client, test_sas_expiry_time_secs, signature, &out_signature)));

  az_span_for_test_verify(
      out_signature, expected_signature, (int32_t)strlen(expected_signature), signature, TEST_SPAN_BUFFER_SIZE);
}

static void az_iot_hub_client_sas_get_password_device_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_DEVICE_HOSTNAME_STR "/devices/" TEST_DEVICE_ID_STR
        "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR;

  az_span key_name = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_true(az_succeeded(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length)));
  
  assert_int_equal(length, _az_COUNTOF(expected_password) - 1);
  assert_memory_equal(password, expected_password, length + 1); // +1 to account for '\0'.
}

static void az_iot_hub_client_sas_get_password_device_no_out_length_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_DEVICE_HOSTNAME_STR "/devices/" TEST_DEVICE_ID_STR
        "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR;

  az_span key_name = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];

  assert_true(az_succeeded(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), NULL)));
  
  assert_memory_equal(password, expected_password, _az_COUNTOF(expected_password)); // Accounts for '\0'.
}

static void az_iot_hub_client_sas_get_password_module_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = test_module_id;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_DEVICE_HOSTNAME_STR "/devices/" TEST_DEVICE_ID_STR
        "/modules/" TEST_MODULE_ID_STR "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR;

  az_span key_name = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_true(az_succeeded(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length)));
  
  assert_int_equal(length, _az_COUNTOF(expected_password) - 1);
  assert_memory_equal(password, expected_password, length + 1); // +1 to account for '\0'.
}

static void az_iot_hub_client_sas_get_password_module_no_length_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = test_module_id;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_DEVICE_HOSTNAME_STR "/devices/" TEST_DEVICE_ID_STR
        "/modules/" TEST_MODULE_ID_STR "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR;

  az_span key_name = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];

  assert_true(az_succeeded(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), NULL)));
  
  assert_memory_equal(password, expected_password, _az_COUNTOF(expected_password)); // Accounts for '\0'.
}

static void az_iot_hub_client_sas_get_password_device_with_keyname_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_DEVICE_HOSTNAME_STR "/devices/" TEST_DEVICE_ID_STR
        "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR "&skn=" TEST_KEY_NAME;

  az_span key_name = AZ_SPAN_FROM_STR(TEST_KEY_NAME);

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_true(az_succeeded(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length)));
  
  assert_int_equal(length, _az_COUNTOF(expected_password) - 1);
  assert_memory_equal(password, expected_password, length + 1); // +1 to account for '\0'.
}

static void az_iot_hub_client_sas_get_password_module_with_keyname_succeeds(void** state)
{
  (void)state;
  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = test_module_id;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  const char expected_password[] = "SharedAccessSignature sr=" TEST_DEVICE_HOSTNAME_STR
                                  "/devices/" TEST_DEVICE_ID_STR "/modules/" TEST_MODULE_ID_STR
                                  "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR "&skn=" TEST_KEY_NAME;

  az_span key_name = AZ_SPAN_FROM_STR(TEST_KEY_NAME);

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_true(az_succeeded(az_iot_hub_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length)));
  
  assert_int_equal(length, _az_COUNTOF(expected_password) - 1);
  assert_memory_equal(password, expected_password, length + 1); // +1 to account for '\0'.
}

static void az_iot_hub_client_sas_get_password_device_overflow_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_span key_name = AZ_SPAN_NULL;

  char password[132];
  size_t length = 0;

  assert_int_equal(
      az_iot_hub_client_sas_get_password(
          &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void az_iot_hub_client_sas_get_password_module_overflow_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = test_module_id;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  az_span key_name = AZ_SPAN_NULL;

  char password[150];
  size_t length = 0;

  assert_int_equal(
      az_iot_hub_client_sas_get_password(
          &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void az_iot_hub_client_sas_get_signature_device_signature_overflow_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  assert_true(az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  uint8_t signature_buffer[54];
  az_span signature = az_span_init(signature_buffer, _az_COUNTOF(signature_buffer));

  assert_int_equal(
      az_iot_hub_client_sas_get_signature(
          &client, test_sas_expiry_time_secs, signature, &signature),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void az_iot_hub_client_sas_get_signature_module_signature_overflow_fails(void** state)
{
  (void)state;
  az_iot_hub_client client;
  az_iot_hub_client_options options;
  options.module_id = test_module_id;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, &options) == AZ_OK);

  uint8_t signature_buffer[72];
  az_span signature = az_span_init(signature_buffer, _az_COUNTOF(signature_buffer));

  assert_int_equal(
      az_iot_hub_client_sas_get_signature(
          &client, test_sas_expiry_time_secs, signature, &signature),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

int test_iot_sas_token()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_NULL_signature_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_NULL_signature_span_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_NULL_client_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_EMPTY_signature_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_NULL_password_span_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_empty_password_buffer_span_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_device_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_device_no_out_length_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_module_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_module_no_length_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_device_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_module_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_device_with_keyname_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_module_with_keyname_succeeds),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_device_overflow_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_password_module_overflow_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_device_signature_overflow_fails),
    cmocka_unit_test(az_iot_hub_client_sas_get_signature_module_signature_overflow_fails)
  };
  return cmocka_run_group_tests_name("az_iot_hub_client_sas", tests, NULL, NULL);
}
