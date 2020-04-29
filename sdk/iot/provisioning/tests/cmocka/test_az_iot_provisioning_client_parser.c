// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_iot_provisioning_client.h>
#include <az_span.h>
#include <az_test_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
#include <_az_cfg.h>

#define TEST_REGISTRATION_ID "myRegistrationId"
#define TEST_HUB_HOSTNAME "contoso.azure-devices.net"
#define TEST_DEVICE_ID "my-device-id1"
#define TEST_OPERATION_ID "4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d"

#define TEST_STATUS_ASSIGNING "assigning"
#define TEST_STATUS_ASSIGNED "assigned"
#define TEST_STATUS_FAILED "failed"
#define TEST_STATUS_DISABLED "disabled"

#define TEST_ERROR_MESSAGE_INVALID_CERT "Invalid certificate."
#define TEST_ERROR_MESSAGE_ALLOCATION "Custom allocation failed with status code: 400"
#define TEST_ERROR_TRACKING_ID "8ad0463c-6427-4479-9dfa-3e8bb7003e9b"
#define TEST_ERROR_TIMESTAMP "2020-04-10T05:24:22.4718526Z"

static void
test_az_iot_provisioning_client_parse_received_topic_and_payload_assigning_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/202/?$rid=1&retry-after=3");
  az_span received_payload = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                                              "\",\"status\":\"" TEST_STATUS_ASSIGNING "\"}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_ACCEPTED, response.status); // 202
  assert_int_equal(3, response.retry_after_seconds);

  // From payload
  assert_memory_equal(
      az_span_ptr(response.operation_id), TEST_OPERATION_ID, strlen(TEST_OPERATION_ID));
  assert_memory_equal(
      az_span_ptr(response.registration_state),
      TEST_STATUS_ASSIGNING,
      strlen(TEST_STATUS_ASSIGNING));
}

static void
test_az_iot_provisioning_client_parse_received_topic_and_payload_topic_not_matched_fails()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/unknown");
  az_span received_payload = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                                              "\",\"status\":\"" TEST_STATUS_ASSIGNING "\"}");

  az_iot_provisioning_client_register_response response = { .status = AZ_IOT_STATUS_FORBIDDEN,
                                                            .retry_after_seconds = 0xBAADC0DE,
                                                            .operation_id = AZ_SPAN_NULL,
                                                            .registration_state = AZ_SPAN_NULL,
                                                            .registration_information = { 0 } };

  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_ERROR_IOT_TOPIC_NO_MATCH, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_FORBIDDEN, response.status); // 202
  assert_int_equal(0xBAADC0DE, response.retry_after_seconds);
}

static void
test_az_iot_provisioning_client_parse_received_topic_and_payload_parse_assigning2_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/202/?retry-after=120&$rid=1");
  az_span received_payload = AZ_SPAN_FROM_STR(
      "{\"operationId\":\"" TEST_OPERATION_ID "\",\"status\":\"" TEST_STATUS_ASSIGNING
      "\",\"registrationState\":{\"registrationId\":\"" TEST_REGISTRATION_ID
      "\",\"status\":\"" TEST_STATUS_ASSIGNING "\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_ACCEPTED, response.status); // 202
  assert_int_equal(120, response.retry_after_seconds);

  // From payload
  assert_memory_equal(
      az_span_ptr(response.operation_id), TEST_OPERATION_ID, strlen(TEST_OPERATION_ID));
  assert_memory_equal(
      az_span_ptr(response.registration_state),
      TEST_STATUS_ASSIGNING,
      strlen(TEST_STATUS_ASSIGNING));
}

static void
test_az_iot_provisioning_client_parse_received_topic_and_payload_assigned_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                         "\",\"status\":\"" TEST_STATUS_ASSIGNED "\",\"registrationState\":{"
                         "\"x509\":{},"
                         "\"registrationId\":\"" TEST_REGISTRATION_ID "\","
                         "\"createdDateTimeUtc\":\"2020-04-10T03:11:13.0276997Z\","
                         "\"assignedHub\":\"" TEST_HUB_HOSTNAME "\","
                         "\"deviceId\":\"" TEST_DEVICE_ID "\","
                         "\"status\":\"" TEST_STATUS_ASSIGNED "\","
                         "\"substatus\":\"initialAssignment\","
                         "\"lastUpdatedDateTimeUtc\":\"2020-04-10T03:11:13.2096201Z\","
                         "\"etag\":\"IjYxMDA4ZDQ2LTAwMDAtMDEwMC0wMDAwLTVlOGZlM2QxMDAwMCI=\","
                         "\"payload\":{\"hello\":\"world\",\"arr\":[1,2,3,4,5,6],\"num\":123}}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_OK, response.status); // 200
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  assert_memory_equal(
      az_span_ptr(response.operation_id), TEST_OPERATION_ID, strlen(TEST_OPERATION_ID));
  assert_memory_equal(
      az_span_ptr(response.registration_state), TEST_STATUS_ASSIGNED, strlen(TEST_STATUS_ASSIGNED));
  assert_memory_equal(
      az_span_ptr(response.registration_information.assigned_hub_hostname),
      TEST_HUB_HOSTNAME,
      strlen(TEST_HUB_HOSTNAME));
  assert_memory_equal(
      az_span_ptr(response.registration_information.device_id),
      TEST_DEVICE_ID,
      strlen(TEST_DEVICE_ID));

  assert_int_equal(200, response.registration_information.status);
  assert_int_equal(0, response.registration_information.extended_error_code);
  assert_int_equal(0, az_span_size(response.registration_information.error_message));
}

static void
test_az_iot_provisioning_client_parse_received_topic_and_payload_invalid_certificate_error_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/401/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"errorCode\":401002,\"trackingId\":\"" TEST_ERROR_TRACKING_ID "\","
                         "\"message\":\"" TEST_ERROR_MESSAGE_INVALID_CERT
                         "\",\"timestampUtc\":\"" TEST_ERROR_TIMESTAMP "\"}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_UNAUTHORIZED, response.status); // 401
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  assert_int_equal(0, az_span_size(response.operation_id));
  assert_memory_equal(
      az_span_ptr(response.registration_state), TEST_STATUS_FAILED, strlen(TEST_STATUS_FAILED));

  assert_int_equal(0, az_span_size(response.registration_information.assigned_hub_hostname));
  assert_int_equal(0, az_span_size(response.registration_information.device_id));

  assert_int_equal(AZ_IOT_STATUS_UNAUTHORIZED, response.registration_information.status);
  assert_int_equal(401002, response.registration_information.extended_error_code);

  assert_memory_equal(
      az_span_ptr(response.registration_information.error_message),
      TEST_ERROR_MESSAGE_INVALID_CERT,
      strlen(TEST_ERROR_MESSAGE_INVALID_CERT));
  assert_memory_equal(
      az_span_ptr(response.registration_information.error_timestamp),
      TEST_ERROR_TIMESTAMP,
      strlen(TEST_ERROR_TIMESTAMP));
  assert_memory_equal(
      az_span_ptr(response.registration_information.error_tracking_id),
      TEST_ERROR_TRACKING_ID,
      strlen(TEST_ERROR_TRACKING_ID));
}

static void test_az_iot_provisioning_client_parse_received_topic_payload_disabled_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"operationId\":\"\",\"status\":\"" TEST_STATUS_DISABLED
                         "\",\"registrationState\":{\"registrationId\":\"" TEST_REGISTRATION_ID
                         "\",\"status\":\"" TEST_STATUS_DISABLED "\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_OK, response.status); // 200
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  assert_int_equal(0, az_span_size(response.operation_id));
  assert_memory_equal(
      az_span_ptr(response.registration_state), TEST_STATUS_DISABLED, strlen(TEST_STATUS_DISABLED));
}

static void
test_az_iot_provisioning_client_parse_received_topic_and_payload_allocation_error_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                         "\",\"status\":\"" TEST_STATUS_FAILED "\",\"registrationState\":{"
                         "\"registrationId\":\"" TEST_REGISTRATION_ID "\","
                         "\"createdDateTimeUtc\":\"2020-04-10T03:11:13.0276997Z\","
                         "\"status\":\"" TEST_STATUS_FAILED "\","
                         "\"errorCode\":400207,"
                         "\"errorMessage\":\"" TEST_ERROR_MESSAGE_ALLOCATION "\","
                         "\"lastUpdatedDateTimeUtc\":\"" TEST_ERROR_TIMESTAMP "\","
                         "\"etag\":\"IjYxMDA4ZDQ2LTAwMDAtMDEwMC0wMDAwLTVlOGZlM2QxMDAwMCI=\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic (the last request succeeded, entire operation failed)
  assert_int_equal(AZ_IOT_STATUS_OK, response.status); // 200
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  assert_memory_equal(
      az_span_ptr(response.operation_id), TEST_OPERATION_ID, strlen(TEST_OPERATION_ID));
  assert_memory_equal(
      az_span_ptr(response.registration_state), TEST_STATUS_FAILED, strlen(TEST_STATUS_FAILED));

  assert_int_equal(0, az_span_size(response.registration_information.assigned_hub_hostname));
  assert_int_equal(0, az_span_size(response.registration_information.device_id));

  assert_int_equal(AZ_IOT_STATUS_BAD_REQUEST, response.registration_information.status);
  assert_int_equal(400207, response.registration_information.extended_error_code);

  assert_memory_equal(
      az_span_ptr(response.registration_information.error_message),
      TEST_ERROR_MESSAGE_ALLOCATION,
      strlen(TEST_ERROR_MESSAGE_ALLOCATION));
  assert_memory_equal(
      az_span_ptr(response.registration_information.error_timestamp),
      TEST_ERROR_TIMESTAMP,
      strlen(TEST_ERROR_TIMESTAMP));

  assert_int_equal(0, az_span_size(response.registration_information.error_tracking_id));
}

static void
test_az_iot_provisioning_client_received_topic_and_payload_parse_invalid_json_payload_fails()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload = AZ_SPAN_FROM_STR("123");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_ERROR_PARSER_UNEXPECTED_CHAR, ret);
}

static void
test_az_iot_provisioning_client_received_topic_and_payload_parse_operationid_not_found_fails()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR(/*\"operationId\":\"" TEST_OPERATION_ID*/
                         "{\"status\":\"" TEST_STATUS_FAILED "\",\"registrationState\":{"
                         "\"registrationId\":\"" TEST_REGISTRATION_ID "\","
                         "\"createdDateTimeUtc\":\"2020-04-10T03:11:13.0276997Z\","
                         "\"status\":\"" TEST_STATUS_FAILED "\","
                         "\"errorCode\":400207,"
                         "\"errorMessage\":\"" TEST_ERROR_MESSAGE_ALLOCATION "\","
                         "\"lastUpdatedDateTimeUtc\":\"" TEST_ERROR_TIMESTAMP "\","
                         "\"etag\":\"IjYxMDA4ZDQ2LTAwMDAtMDEwMC0wMDAwLTVlOGZlM2QxMDAwMCI=\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_ERROR_ITEM_NOT_FOUND, ret);
}

static void
test_az_iot_provisioning_client_received_topic_and_payload_parse_registration_state_not_found_fails()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                         /*"\",\"status\":\"" TEST_STATUS_FAILED */ "\",\"registrationState\":{"
                         "\"registrationId\":\"" TEST_REGISTRATION_ID "\","
                         "\"createdDateTimeUtc\":\"2020-04-10T03:11:13.0276997Z\","
                         "\"status\":\"" TEST_STATUS_FAILED "\","
                         "\"errorCode\":400207,"
                         "\"errorMessage\":\"" TEST_ERROR_MESSAGE_ALLOCATION "\","
                         "\"lastUpdatedDateTimeUtc\":\"" TEST_ERROR_TIMESTAMP "\","
                         "\"etag\":\"IjYxMDA4ZDQ2LTAwMDAtMDEwMC0wMDAwLTVlOGZlM2QxMDAwMCI=\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_ERROR_ITEM_NOT_FOUND, ret);
}

static void
test_az_iot_provisioning_client_received_topic_and_payload_parse_error_code_not_found_fails()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload = AZ_SPAN_FROM_STR(
      "{" /*\"errorCode\":401002,*/ "\"trackingId\":\"" TEST_ERROR_TRACKING_ID "\","
      "\"message\":\"" TEST_ERROR_MESSAGE_INVALID_CERT "\",\"timestampUtc\":\"" TEST_ERROR_TIMESTAMP
      "\"}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_ERROR_ITEM_NOT_FOUND, ret);
}

static void
test_az_iot_provisioning_client_received_topic_and_payload_parse_invalid_registration_state_fails()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                         "\",\"status\":\"" TEST_STATUS_FAILED "\",\"registrationState\":123}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_ERROR_PARSER_UNEXPECTED_CHAR, ret);
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_provisioning_client_parser()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_and_payload_assigning_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_and_payload_topic_not_matched_fails),
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_and_payload_parse_assigning2_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_and_payload_assigned_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_and_payload_invalid_certificate_error_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_payload_disabled_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_parse_received_topic_and_payload_allocation_error_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_and_payload_parse_invalid_json_payload_fails),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_and_payload_parse_operationid_not_found_fails),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_and_payload_parse_registration_state_not_found_fails),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_and_payload_parse_error_code_not_found_fails),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_and_payload_parse_invalid_registration_state_fails),
  };

  return cmocka_run_group_tests_name("az_iot_provisioning_client_parser", tests, NULL, NULL);
}
