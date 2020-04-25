// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_iot_provisioning_client.h>
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

#define TEST_REGISTRATION_ID_STR "myRegistrationId"
#define TEST_ID_SCOPE "0neFEEDC0DE"
#define TEST_URL_ENCODED_RESOURCE_URI "0neFEEDC0DE%2fregistrations%2fmyRegistrationId"

#define TEST_SIG "cS1eHM%2FlDjsRsrZV9508wOFrgmZk4g8FNg8NwHVSiSQ"
#define TEST_EXPIRATION_STR "1578941692"
#define TEST_KEY_NAME "iothubowner"

static const az_span test_global_device_endpoint = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");
static const az_span test_id_scope = AZ_SPAN_LITERAL_FROM_STR(TEST_ID_SCOPE);
static const az_span test_registration_id = AZ_SPAN_LITERAL_FROM_STR(TEST_REGISTRATION_ID_STR);
static const uint32_t test_sas_expiry_time_secs = 1578941692;
static const az_span test_signature = AZ_SPAN_LITERAL_FROM_STR(TEST_SIG);

#ifndef NO_PRECONDITION_CHECKING

enable_precondition_check_tests()

// Tests

static void az_iot_provisioning_client_sas_get_signature_NULL_signature_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  az_span signature = AZ_SPAN_NULL;

  assert_precondition_checked(
      az_iot_provisioning_client_sas_get_signature(&client, test_sas_expiry_time_secs, signature, NULL));
}

static void az_iot_provisioning_client_sas_get_signature_NULL_signature_span_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  az_span signature = AZ_SPAN_NULL;

  assert_precondition_checked(az_iot_provisioning_client_sas_get_signature(
      &client, test_sas_expiry_time_secs, signature, &signature));
}

static void az_iot_provisioning_client_sas_get_signature_NULL_client_fails(void** state)
{
  (void)state;
  uint8_t signature_buffer[TEST_SPAN_BUFFER_SIZE];
  az_span signature = az_span_init(signature_buffer, _az_COUNTOF(signature_buffer));

  assert_precondition_checked(
      az_iot_provisioning_client_sas_get_signature(NULL, test_sas_expiry_time_secs, signature, &signature));
}

static void az_iot_provisioning_client_sas_get_password_EMPTY_signature_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_precondition_checked(az_iot_provisioning_client_sas_get_password(
      &client, signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length));
}

static void az_iot_provisioning_client_sas_get_password_NULL_password_span_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  az_span key_name = AZ_SPAN_NULL;
  size_t length = 0;
  char password[TEST_SPAN_BUFFER_SIZE];

  assert_precondition_checked(az_iot_provisioning_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, NULL, _az_COUNTOF(password), &length));
}

static void az_iot_provisioning_client_sas_get_password_empty_password_buffer_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  az_span key_name = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_precondition_checked(az_iot_provisioning_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, 0, &length));
}

#endif // NO_PRECONDITION_CHECKING

static void az_iot_provisioning_client_sas_get_signature_device_succeeds(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  const char expected_signature[]
      = TEST_URL_ENCODED_RESOURCE_URI "\n" TEST_EXPIRATION_STR;

  uint8_t signature_buffer[TEST_SPAN_BUFFER_SIZE];
  az_span signature = az_span_for_test_init(signature_buffer, _az_COUNTOF(signature_buffer));
  az_span out_signature;

  assert_true(az_succeeded(az_iot_provisioning_client_sas_get_signature(
      &client, test_sas_expiry_time_secs, signature, &out_signature)));

  az_span_for_test_verify(
      out_signature, expected_signature, _az_COUNTOF(expected_signature) - 1, signature, TEST_SPAN_BUFFER_SIZE);
}

static void az_iot_provisioning_client_sas_get_password_device_succeeds(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_ID_SCOPE "%2fregistrations%2f" TEST_REGISTRATION_ID_STR
        "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR;

  az_span key_name = AZ_SPAN_NULL;

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_true(az_succeeded(az_iot_provisioning_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length)));
  
  assert_int_equal(length, _az_COUNTOF(expected_password) - 1);
  assert_memory_equal(password, expected_password, length + 1); // +1 to account for '\0'.
}

static void az_iot_provisioning_client_sas_get_password_device_with_keyname_succeeds(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  const char expected_password[]
      = "SharedAccessSignature sr=" TEST_ID_SCOPE "%2fregistrations%2f" TEST_REGISTRATION_ID_STR
        "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR "&skn=" TEST_KEY_NAME;

  az_span key_name = AZ_SPAN_FROM_STR(TEST_KEY_NAME);

  char password[TEST_SPAN_BUFFER_SIZE];
  size_t length = 0;

  assert_true(az_succeeded(az_iot_provisioning_client_sas_get_password(
      &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length)));

  assert_int_equal(length, _az_COUNTOF(expected_password) - 1);
  assert_memory_equal(password, expected_password, length + 1); // +1 to account for '\0'.
}

static void az_iot_provisioning_client_sas_get_password_device_overflow_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  az_span key_name = AZ_SPAN_NULL;

  char password[132];
  size_t length = 0;

  assert_int_equal(
      az_iot_provisioning_client_sas_get_password(
          &client, test_signature, test_sas_expiry_time_secs, key_name, password, _az_COUNTOF(password), &length),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

static void az_iot_provisioning_client_sas_get_signature_device_signature_overflow_fails(void** state)
{
  (void)state;
  az_iot_provisioning_client client;
  assert_int_equal(az_iot_provisioning_client_init(&client, test_global_device_endpoint, test_id_scope, test_registration_id, NULL), AZ_OK);

  uint8_t signature_buffer[54];
  az_span signature = az_span_init(signature_buffer, _az_COUNTOF(signature_buffer));

  assert_int_equal(
      az_iot_provisioning_client_sas_get_signature(
          &client, test_sas_expiry_time_secs, signature, &signature),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

int test_az_iot_provisioning_client_sas_token()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(az_iot_provisioning_client_sas_get_signature_NULL_signature_fails),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_signature_NULL_signature_span_fails),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_signature_NULL_client_fails),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_password_EMPTY_signature_fails),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_password_NULL_password_span_fails),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_password_empty_password_buffer_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(az_iot_provisioning_client_sas_get_signature_device_succeeds),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_password_device_succeeds),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_password_device_with_keyname_succeeds),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_password_device_overflow_fails),
    cmocka_unit_test(az_iot_provisioning_client_sas_get_signature_device_signature_overflow_fails)
  };
  return cmocka_run_group_tests_name("az_iot_provisioning_client_sas", tests, NULL, NULL);
}
