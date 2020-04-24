// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_iot_sas_token.h>
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

#define TEST_DEVICEID "mytest_deviceid"
#define TEST_FQDN "myiothub.azure-devices.net"
#define TEST_SIG "cS1eHM%2FlDjsRsrZV9508wOFrgmZk4g8FNg8NwHVSiSQ"
#define TEST_EXPIRATION 1578941692
#define TEST_EXPIRATION_STR "1578941692"
#define TEST_KEY_NAME "iothubowner"

#ifndef NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

static void az_iot_sas_token_get_document_NULL_document_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span document = AZ_SPAN_NULL;

  assert_precondition_checked(
      az_iot_sas_token_get_document(iothub_fqdn, device_id, expiry_time_secs, document, NULL));
}

static void az_iot_sas_token_get_document_NULL_document_span_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;

  az_span document = AZ_SPAN_NULL;

  assert_precondition_checked(
      az_iot_sas_token_get_document(iothub_fqdn, device_id, expiry_time_secs, document, &document));
}

static void az_iot_sas_token_get_document_empty_device_id_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_NULL;
  int32_t expiry_time_secs = TEST_EXPIRATION;

  uint8_t raw_document[TEST_SPAN_BUFFER_SIZE];
  az_span document = az_span_init(raw_document, _az_COUNTOF(raw_document));

  assert_precondition_checked(
      az_iot_sas_token_get_document(iothub_fqdn, device_id, expiry_time_secs, document, &document));
}

static void az_iot_sas_token_get_document_empty_iothub_fqdn_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_NULL;
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;

  uint8_t raw_document[TEST_SPAN_BUFFER_SIZE];
  az_span document = az_span_init(raw_document, _az_COUNTOF(raw_document));

  assert_precondition_checked(
      az_iot_sas_token_get_document(iothub_fqdn, device_id, expiry_time_secs, document, &document));
}

static void az_iot_sas_token_generate_empty_device_id_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_NULL;
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  uint8_t raw_sas_token[TEST_SPAN_BUFFER_SIZE];
  az_span sas_token = az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_precondition_checked(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token));
}

static void az_iot_sas_token_generate_empty_iothub_fqdn_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_NULL;
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  uint8_t raw_sas_token[TEST_SPAN_BUFFER_SIZE];
  az_span sas_token = az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_precondition_checked(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token));
}

static void az_iot_sas_token_generate_EMPTY_signature_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_NULL;

  uint8_t raw_sas_token[TEST_SPAN_BUFFER_SIZE];
  az_span sas_token = az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_precondition_checked(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token));
}

static void az_iot_sas_token_generate_NULL_sas_token_span_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  az_span sas_token = AZ_SPAN_NULL;

  assert_precondition_checked(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token));
}

static void az_iot_sas_token_generate_NULL_out_sas_token_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  uint8_t raw_sas_token[TEST_SPAN_BUFFER_SIZE];
  az_span sas_token = az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_precondition_checked(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, NULL));
}

#endif // NO_PRECONDITION_CHECKING

static void az_iot_sas_token_get_document_succeeds(void** state)
{
  (void)state;
  const char* expected_document = TEST_FQDN "/devices/" TEST_DEVICEID "\n" TEST_EXPIRATION_STR;

  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;

  uint8_t raw_document[TEST_SPAN_BUFFER_SIZE];
  az_span document = az_span_for_test_init(raw_document, _az_COUNTOF(raw_document));

  assert_true(az_succeeded(az_iot_sas_token_get_document(
      iothub_fqdn, device_id, expiry_time_secs, document, &document)));
  az_span_for_test_verify(
      document,
      expected_document,
      (int32_t)strlen(expected_document),
      az_span_init(raw_document, _az_COUNTOF(raw_document)),
      TEST_SPAN_BUFFER_SIZE);
}

static void az_iot_sas_token_generate_succeeds(void** state)
{
  (void)state;
  const char* expected_sas_token = "SharedAccessSignature sr=" TEST_FQDN "/devices/" TEST_DEVICEID
                                   "&sig=" TEST_SIG "&se=" TEST_EXPIRATION_STR;

  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  uint8_t raw_sas_token[TEST_SPAN_BUFFER_SIZE];
  az_span sas_token = az_span_for_test_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_true(az_succeeded(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token)));
  az_span_for_test_verify(
      sas_token,
      expected_sas_token,
      (int32_t)strlen(expected_sas_token),
      az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token)),
      TEST_SPAN_BUFFER_SIZE);
}

static void az_iot_sas_token_generate_with_keyname_succeeds(void** state)
{
  (void)state;
  const char* expected_sas_token
      = "SharedAccessSignature sr=" TEST_FQDN "/devices/" TEST_DEVICEID "&sig=" TEST_SIG
        "&se=" TEST_EXPIRATION_STR "&skn=" TEST_KEY_NAME;

  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_FROM_STR(TEST_KEY_NAME);
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  uint8_t raw_sas_token[TEST_SPAN_BUFFER_SIZE];
  az_span sas_token = az_span_for_test_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_true(az_succeeded(az_iot_sas_token_generate(
      iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token)));
  az_span_for_test_verify(
      sas_token,
      expected_sas_token,
      (int32_t)strlen(expected_sas_token),
      az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token)),
      TEST_SPAN_BUFFER_SIZE);
}

// Conditions like buffer (i.e., az_span) capacity are not covered by pre-conditions, so these tests
// are always mandatory.
static void az_iot_sas_token_generate_sas_token_overflow_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;
  az_span key_name = AZ_SPAN_NULL;
  az_span signature = AZ_SPAN_FROM_STR(TEST_SIG);

  uint8_t raw_sas_token[32];
  az_span sas_token = az_span_init(raw_sas_token, _az_COUNTOF(raw_sas_token));

  assert_int_equal(
      az_iot_sas_token_generate(
          iothub_fqdn, device_id, signature, expiry_time_secs, key_name, sas_token, &sas_token),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

// Conditions like buffer (i.e., az_span) capacity are not covered by pre-conditions, so these tests
// are always mandatory.
static void az_iot_sas_token_get_document_document_overflow_fails(void** state)
{
  (void)state;
  az_span iothub_fqdn = AZ_SPAN_FROM_STR(TEST_FQDN);
  az_span device_id = AZ_SPAN_FROM_STR(TEST_DEVICEID);
  int32_t expiry_time_secs = TEST_EXPIRATION;

  uint8_t raw_document[32];
  az_span document = az_span_init(raw_document, _az_COUNTOF(raw_document));

  assert_int_equal(
      az_iot_sas_token_get_document(iothub_fqdn, device_id, expiry_time_secs, document, &document),
      AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
}

int test_iot_sas_token()
{
#ifndef NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef NO_PRECONDITION_CHECKING
    cmocka_unit_test(az_iot_sas_token_get_document_NULL_document_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_NULL_document_span_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_empty_device_id_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_empty_iothub_fqdn_fails),
    cmocka_unit_test(az_iot_sas_token_generate_empty_device_id_fails),
    cmocka_unit_test(az_iot_sas_token_generate_empty_iothub_fqdn_fails),
    cmocka_unit_test(az_iot_sas_token_generate_EMPTY_signature_fails),
    cmocka_unit_test(az_iot_sas_token_generate_NULL_sas_token_span_fails),
    cmocka_unit_test(az_iot_sas_token_generate_NULL_out_sas_token_fails),
#endif // NO_PRECONDITION_CHECKING
    cmocka_unit_test(az_iot_sas_token_get_document_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_with_keyname_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_sas_token_overflow_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_document_overflow_fails)
  };
  return cmocka_run_group_tests_name("az_iot_hub_client_sas", tests, NULL, NULL);
}
