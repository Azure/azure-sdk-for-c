// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

/*
 * SAS Token Unit Tests
 */
void az_iot_sas_token_get_document_NULL_document_fails(void** state);
void az_iot_sas_token_get_document_NULL_document_span_fails(void** state);
void az_iot_sas_token_get_document_empty_device_id_fails(void** state);
void az_iot_sas_token_get_document_empty_iothub_fqdn_fails(void** state);
void az_iot_sas_token_get_document_document_overflow_fails(void** state); */
void az_iot_sas_token_get_document_succeeds(void** state);
void az_iot_sas_token_generate_empty_device_id_fails(void** state);
void az_iot_sas_token_generate_empty_iothub_fqdn_fails(void** state);
void az_iot_sas_token_generate_EMPTY_signature_fails(void** state);
void az_iot_sas_token_generate_NULL_sas_token_fails(void** state);
void az_iot_sas_token_generate_NULL_sas_token_span_fails(void** state);
void az_iot_sas_token_generate_sas_token_overflow_fails(void** state); */
void az_iot_sas_token_generate_succeeds(void** state);
void az_iot_sas_token_generate_with_keyname_succeeds(void** state);

/*
 * Telemetry Unit Tests
 */
void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_client_fails(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_mqtt_topic_fails(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_NULL_out_mqtt_topic_fails(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_no_params_succeed(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_no_params_succeed(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_succeed(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_small_buffer_fails(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_succeed(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_small_buffer_fails(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_succeed(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_small_buffer_fails(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_succeed(void** state);
void test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_small_buffer_fails(void** state);

int main()
{
  const struct CMUnitTest tests[] = {
    // SAS Token
    cmocka_unit_test(az_iot_sas_token_get_document_NULL_document_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_NULL_document_span_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_empty_device_id_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_empty_iothub_fqdn_fails),
    cmocka_unit_test(az_iot_sas_token_get_document_document_overflow_fails), */
    cmocka_unit_test(az_iot_sas_token_get_document_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_empty_device_id_fails),
    cmocka_unit_test(az_iot_sas_token_generate_empty_iothub_fqdn_fails),
    cmocka_unit_test(az_iot_sas_token_generate_EMPTY_signature_fails),
    cmocka_unit_test(az_iot_sas_token_generate_NULL_sas_token_fails),
    cmocka_unit_test(az_iot_sas_token_generate_NULL_sas_token_span_fails),
    cmocka_unit_test(az_iot_sas_token_generate_sas_token_overflow_fails),
    cmocka_unit_test(az_iot_sas_token_generate_succeeds),
    cmocka_unit_test(az_iot_sas_token_generate_with_keyname_succeeds),

    // Telemetry
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_NULL_client_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_NULL_mqtt_topic_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_NULL_out_mqtt_topic_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_no_options_no_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_no_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_with_params_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_no_options_with_params_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_module_id_with_params_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_succeed),
    cmocka_unit_test(test_az_iot_hub_client_telemetry_publish_topic_get_with_options_user_agent_with_params_small_buffer_fails),
  };

  return cmocka_run_group_tests_name("az_iot", tests, NULL, NULL);
}
