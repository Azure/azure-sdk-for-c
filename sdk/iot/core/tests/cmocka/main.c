// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>

#include <cmocka.h>

void az_iot_sas_token_get_document_NULL_document_fails(void** state);
void az_iot_sas_token_get_document_NULL_document_span_fails(void** state);
void az_iot_sas_token_get_document_empty_device_id_fails(void** state);
void az_iot_sas_token_get_document_empty_iothub_fqdn_fails(void** state);
void az_iot_sas_token_get_document_document_overflow_fails(void** state);
void az_iot_sas_token_get_document_succeeds(void** state);
void az_iot_sas_token_generate_empty_device_id_fails(void** state);
void az_iot_sas_token_generate_empty_iothub_fqdn_fails(void** state);
void az_iot_sas_token_generate_EMPTY_signature_fails(void** state);
void az_iot_sas_token_generate_NULL_sas_token_fails(void** state);
void az_iot_sas_token_generate_NULL_sas_token_span_fails(void** state);
void az_iot_sas_token_generate_sas_token_overflow_fails(void** state);
void az_iot_sas_token_generate_succeeds(void** state);
void az_iot_sas_token_generate_with_keyname_succeeds(void** state);

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(az_iot_sas_token_get_document_NULL_document_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_NULL_document_span_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_empty_device_id_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_empty_iothub_fqdn_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_document_overflow_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_empty_device_id_fails),
    cmocka_unit_test(az_iot_sas_token_generate_empty_iothub_fqdn_fails),
    cmocka_unit_test(az_iot_sas_token_generate_EMPTY_signature_fails),
    cmocka_unit_test(az_iot_sas_token_generate_NULL_sas_token_fails),
    cmocka_unit_test(az_iot_sas_token_generate_NULL_sas_token_span_fails),
    cmocka_unit_test(az_iot_sas_token_generate_sas_token_overflow_fails),
    cmocka_unit_test(az_iot_sas_token_generate_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_with_keyname_succeeds),
  };

  return cmocka_run_group_tests_name("az_iot", tests, NULL, NULL);
}
