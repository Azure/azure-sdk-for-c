// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_hub_client.h"
#include <az_test_log.h>
#include <az_test_precondition.h>
#include <az_test_span.h>
#include <azure/core/az_log.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_hub_client.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 256

static const az_span test_device_id = AZ_SPAN_LITERAL_FROM_STR("my_device");
static const az_span test_device_hostname = AZ_SPAN_LITERAL_FROM_STR("myiothub.azure-devices.net");
static const az_span test_device_request_id = AZ_SPAN_LITERAL_FROM_STR("req-001");
static const az_span test_csr_base64 = AZ_SPAN_LITERAL_FROM_STR("MIICYTCCAUkCAQAwHDEaMBgGA1wR");

// Expected publish topic
static const char test_correct_certificate_signing_request_topic[]
    = "$iothub/credentials/POST/issueCertificate/?$rid=req-001";

// Received topics for parsing
static const az_span test_certificate_signing_requests_accepted_topic_202
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/res/202/?$rid=req-001");
static const az_span test_certifivate_signing_response_topic_200
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/res/200/?$rid=req-001");
static const az_span test_certifivate_signing_response_topic_400
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/res/400/?$rid=req-001");
static const az_span test_certifivate_signing_response_topic_429
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/res/429/?$rid=req-001");
static const az_span test_certifivate_signing_response_received_topic_503
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/res/503/?$rid=req-001");
static const az_span test_certifivate_signing_response_topic_no_match
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/res/200/?$rid=req-001");
static const az_span test_certifivate_signing_response_topic_incomplete
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/res/200");

// Accepted (202) response payload
static const az_span test_accepted_payload = AZ_SPAN_LITERAL_FROM_STR(
    "{\"correlationId\":\"8819e8d8-1324-4a9c-acde-ce0318e93f31\","
    "\"operationExpires\":\"2025-06-09T17:31:31.426Z\"}");
static const az_span test_accepted_payload_with_unknown = AZ_SPAN_LITERAL_FROM_STR(
    "{\"correlationId\":\"corr-id\","
    "\"unknownField\":42,"
    "\"operationExpires\":\"2025-06-09T17:31:31.426Z\"}");

// Error response payloads
static const az_span test_error_400040_payload = AZ_SPAN_LITERAL_FROM_STR(
    "{\"errorCode\":400040,"
    "\"message\":\"A Certificate signing request operation failed\","
    "\"trackingId\":\"59b2922c-f1c9-451b-b02d-5b64bc31685a\","
    "\"timestampUtc\":\"2025-06-09T17:31:31.426574675Z\","
    "\"info\":{\"correlationId\":\"8819e8d8-1324-4a9c-acde-ce0318e93f31\","
    "\"credentialError\":\"FailedToDecodeCsr\","
    "\"credentialMessage\":\"Failed to decode CSR: invalid base64 encoding\"}}");

static const az_span test_error_409005_payload = AZ_SPAN_LITERAL_FROM_STR(
    "{\"errorCode\":409005,"
    "\"message\":\"A Certificate signing request operation is already active\","
    "\"trackingId\":\"59b2922c-f1c9-451b-b02d-5b64bc31685a\","
    "\"timestampUtc\":\"2025-06-09T17:31:31.426574675Z\","
    "\"info\":{\"requestId\":\"aabbcc\","
    "\"correlationId\":\"8819e8d8-1324-4a9c-acde-ce0318e93f31\","
    "\"operationExpires\":\"2025-06-09T17:31:31.426Z\"}}");

static const az_span test_error_with_retry_after_payload = AZ_SPAN_LITERAL_FROM_STR(
    "{\"errorCode\":429002,"
    "\"message\":\"Throttled\","
    "\"trackingId\":\"track-id\","
    "\"timestampUtc\":\"2025-06-09T17:31:31Z\","
    "\"retryAfter\":5}");

static const az_span test_error_no_info_payload = AZ_SPAN_LITERAL_FROM_STR(
    "{\"errorCode\":503001,"
    "\"message\":\"Service unavailable\","
    "\"trackingId\":\"track-id\","
    "\"timestampUtc\":\"2025-06-09T17:31:31Z\"}");

// ---- Precondition tests ----

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

// get_publish_topic precondition tests
static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_NULL_client_fails(
    void** state)
{
  (void)state;

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_publish_topic(
      NULL, test_device_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_NULL_request_id_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  uint8_t test_bad_request_id_buf[1];
  az_span test_bad_request_id
      = az_span_create(test_bad_request_id_buf, _az_COUNTOF(test_bad_request_id_buf));
  test_bad_request_id._internal.ptr = NULL;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_publish_topic(
      &client, test_bad_request_id, test_buf, sizeof(test_buf), &test_length));
}

static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_NULL_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_publish_topic(
      &client, test_device_request_id, NULL, 128, &test_length));
}

static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_zero_size_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_publish_topic(
      &client, test_device_request_id, test_buf, 0, &test_length));
}

// parse_received_topic precondition tests
static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_NULL_client_fails(void** state)
{
  (void)state;

  az_iot_hub_client_certificate_signing_response_info response_info;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_parse_received_topic(
      NULL, test_certificate_signing_requests_accepted_topic_202, &response_info));
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_NULL_topic_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  uint8_t test_bad_topic_buf[1];
  az_span test_bad_topic
      = az_span_create(test_bad_topic_buf, _az_COUNTOF(test_bad_topic_buf));
  test_bad_topic._internal.ptr = NULL;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(&client, test_bad_topic, &response_info));
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_NULL_response_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_parse_received_topic(
      &client, test_certificate_signing_requests_accepted_topic_202, NULL));
}

// get_request_payload precondition tests
static void test_az_iot_hub_client_certificate_signing_request_get_request_payload_NULL_client_fails(void** state)
{
  (void)state;

  az_iot_hub_client_certificate_signing_request certificate_signing_request
      = { .csr = test_csr_base64 };

  uint8_t payload_buf[TEST_SPAN_BUFFER_SIZE];
  size_t payload_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_request_payload(
      NULL, &certificate_signing_request, payload_buf, sizeof(payload_buf), &payload_length));
}

static void test_az_iot_hub_client_certificate_signing_request_get_request_payload_NULL_options_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  uint8_t payload_buf[TEST_SPAN_BUFFER_SIZE];
  size_t payload_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_request_payload(
      &client, NULL, payload_buf, sizeof(payload_buf), &payload_length));
}

static void test_az_iot_hub_client_certificate_signing_request_get_request_payload_NULL_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_request certificate_signing_request
      = { .csr = test_csr_base64 };

  size_t payload_length;

  ASSERT_PRECONDITION_CHECKED(az_iot_hub_client_certificate_signing_request_get_request_payload(
      &client, &certificate_signing_request, NULL, 128, &payload_length));
}

#endif // AZ_NO_PRECONDITION_CHECKING

// ---- Publish topic tests ----

static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[TEST_SPAN_BUFFER_SIZE];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_buf, test_correct_certificate_signing_request_topic);
  assert_int_equal(test_length, sizeof(test_correct_certificate_signing_request_topic) - 1);
}

static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_small_buffer_fails(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[10];
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_iot_hub_client_certificate_signing_request_get_publish_topic_exact_size_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  char test_buf[sizeof(test_correct_certificate_signing_request_topic)]; // Exact size including null.
  size_t test_length;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_get_publish_topic(
          &client, test_device_request_id, test_buf, sizeof(test_buf), &test_length),
      AZ_OK);

  assert_string_equal(test_buf, test_correct_certificate_signing_request_topic);
  assert_int_equal(test_length, sizeof(test_correct_certificate_signing_request_topic) - 1);
}

// ---- Topic parsing tests ----

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_202_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certificate_signing_requests_accepted_topic_202, &response_info),
      AZ_OK);

  assert_true(az_span_is_content_equal(response_info.request_id, test_device_request_id));
  assert_int_equal(response_info.status_code, AZ_IOT_STATUS_ACCEPTED);
  assert_int_equal(response_info.response_type, AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ACCEPTED);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_200_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certifivate_signing_response_topic_200, &response_info),
      AZ_OK);

  assert_true(az_span_is_content_equal(response_info.request_id, test_device_request_id));
  assert_int_equal(response_info.status_code, AZ_IOT_STATUS_OK);
  assert_int_equal(response_info.response_type, AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_COMPLETED);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_400_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certifivate_signing_response_topic_400, &response_info),
      AZ_OK);

  assert_true(az_span_is_content_equal(response_info.request_id, test_device_request_id));
  assert_int_equal(response_info.status_code, AZ_IOT_STATUS_BAD_REQUEST);
  assert_int_equal(response_info.response_type, AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ERROR);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_429_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certifivate_signing_response_topic_429, &response_info),
      AZ_OK);

  assert_true(az_span_is_content_equal(response_info.request_id, test_device_request_id));
  assert_int_equal(response_info.status_code, AZ_IOT_STATUS_THROTTLED);
  assert_int_equal(response_info.response_type, AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ERROR);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_503_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certifivate_signing_response_received_topic_503, &response_info),
      AZ_OK);

  assert_true(az_span_is_content_equal(response_info.request_id, test_device_request_id));
  assert_int_equal(response_info.status_code, AZ_IOT_STATUS_SERVICE_UNAVAILABLE);
  assert_int_equal(response_info.response_type, AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ERROR);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_no_match_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certifivate_signing_response_topic_no_match, &response_info),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_received_topic_incomplete_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certifivate_signing_response_topic_incomplete, &response_info),
      AZ_ERROR_UNEXPECTED_END);
}

// ---- Request payload tests ----

static void test_az_iot_hub_client_certificate_signing_request_get_request_payload_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_request certificate_signing_request
      = { .csr = test_csr_base64 };

  uint8_t payload_buf[TEST_SPAN_BUFFER_SIZE];
  size_t payload_length;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_get_request_payload(
          &client, &certificate_signing_request, payload_buf, sizeof(payload_buf), &payload_length),
      AZ_OK);

  az_span payload_span = az_span_create(payload_buf, (int32_t)payload_length);
  az_span expected = AZ_SPAN_FROM_STR(
      "{\"id\":\"my_device\",\"csr\":\"MIICYTCCAUkCAQAwHDEaMBgGA1wR\"}");
  assert_true(az_span_is_content_equal(payload_span, expected));
}

static void test_az_iot_hub_client_certificate_signing_request_get_request_payload_with_replace_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_request certificate_signing_request
      = { .csr = test_csr_base64, .replace = AZ_SPAN_FROM_STR("*") };

  uint8_t payload_buf[TEST_SPAN_BUFFER_SIZE];
  size_t payload_length;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_get_request_payload(
          &client, &certificate_signing_request, payload_buf, sizeof(payload_buf), &payload_length),
      AZ_OK);

  az_span payload_span = az_span_create(payload_buf, (int32_t)payload_length);
  az_span expected = AZ_SPAN_FROM_STR(
      "{\"id\":\"my_device\",\"csr\":\"MIICYTCCAUkCAQAwHDEaMBgGA1wR\",\"replace\":\"*\"}");
  assert_true(az_span_is_content_equal(payload_span, expected));
}

static void test_az_iot_hub_client_certificate_signing_request_get_request_payload_small_buffer_fails(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_request certificate_signing_request
      = { .csr = test_csr_base64 };

  uint8_t payload_buf[5];
  size_t payload_length;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_get_request_payload(
          &client, &certificate_signing_request, payload_buf, sizeof(payload_buf), &payload_length),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

// ---- Accepted response parsing tests ----

static void test_az_iot_hub_client_certificate_signing_request_parse_accepted_response_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_accepted_response response;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_accepted_response(&client, test_accepted_payload, &response),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      response.correlation_id,
      AZ_SPAN_FROM_STR("8819e8d8-1324-4a9c-acde-ce0318e93f31")));
  assert_true(az_span_is_content_equal(
      response.operation_expires, AZ_SPAN_FROM_STR("2025-06-09T17:31:31.426Z")));
}

static void test_az_iot_hub_client_certificate_signing_request_parse_accepted_response_unknown_fields_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_accepted_response response;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_accepted_response(
          &client, test_accepted_payload_with_unknown, &response),
      AZ_OK);

  assert_true(
      az_span_is_content_equal(response.correlation_id, AZ_SPAN_FROM_STR("corr-id")));
  assert_true(az_span_is_content_equal(
      response.operation_expires, AZ_SPAN_FROM_STR("2025-06-09T17:31:31.426Z")));
}

// ---- Error response parsing tests ----

static void test_az_iot_hub_client_certificate_signing_request_parse_error_response_400040_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_error_response response;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_error_response(
          &client, test_error_400040_payload, &response),
      AZ_OK);

  assert_int_equal(response.error_code, 400040);
  assert_true(az_span_is_content_equal(
      response.message, AZ_SPAN_FROM_STR("A Certificate signing request operation failed")));
  assert_true(az_span_is_content_equal(
      response.tracking_id, AZ_SPAN_FROM_STR("59b2922c-f1c9-451b-b02d-5b64bc31685a")));
  assert_true(az_span_is_content_equal(
      response.timestamp_utc, AZ_SPAN_FROM_STR("2025-06-09T17:31:31.426574675Z")));
  assert_true(az_span_is_content_equal(
      response.correlation_id, AZ_SPAN_FROM_STR("8819e8d8-1324-4a9c-acde-ce0318e93f31")));
  assert_true(az_span_is_content_equal(
      response.info_error, AZ_SPAN_FROM_STR("FailedToDecodeCsr")));
  assert_true(az_span_is_content_equal(
      response.info_message,
      AZ_SPAN_FROM_STR("Failed to decode CSR: invalid base64 encoding")));
  assert_true(az_span_is_content_equal(response.info_request_id, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_operation_expires, AZ_SPAN_EMPTY));
  assert_int_equal(response.retry_after_seconds, 0);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_error_response_409005_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_error_response response;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_error_response(
          &client, test_error_409005_payload, &response),
      AZ_OK);

  assert_int_equal(response.error_code, 409005);
  assert_true(az_span_is_content_equal(
      response.message, AZ_SPAN_FROM_STR("A Certificate signing request operation is already active")));
  assert_true(az_span_is_content_equal(
      response.tracking_id, AZ_SPAN_FROM_STR("59b2922c-f1c9-451b-b02d-5b64bc31685a")));
  assert_true(az_span_is_content_equal(
      response.correlation_id, AZ_SPAN_FROM_STR("8819e8d8-1324-4a9c-acde-ce0318e93f31")));
  assert_true(
      az_span_is_content_equal(response.info_request_id, AZ_SPAN_FROM_STR("aabbcc")));
  assert_true(az_span_is_content_equal(
      response.info_operation_expires, AZ_SPAN_FROM_STR("2025-06-09T17:31:31.426Z")));
  assert_true(az_span_is_content_equal(response.info_error, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_message, AZ_SPAN_EMPTY));
  assert_int_equal(response.retry_after_seconds, 0);
}

static void test_az_iot_hub_client_certificate_signing_request_parse_error_response_retry_after_succeed(
    void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_error_response response;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_error_response(
          &client, test_error_with_retry_after_payload, &response),
      AZ_OK);

  assert_int_equal(response.error_code, 429002);
  assert_int_equal(response.retry_after_seconds, 5);
  assert_true(az_span_is_content_equal(response.correlation_id, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_error, AZ_SPAN_EMPTY));
}

static void test_az_iot_hub_client_certificate_signing_request_parse_error_response_no_info_succeed(void** state)
{
  (void)state;

  az_iot_hub_client client;
  assert_int_equal(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL), AZ_OK);

  az_iot_hub_client_certificate_signing_error_response response;

  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_error_response(
          &client, test_error_no_info_payload, &response),
      AZ_OK);

  assert_int_equal(response.error_code, 503001);
  assert_true(az_span_is_content_equal(
      response.message, AZ_SPAN_FROM_STR("Service unavailable")));
  assert_true(az_span_is_content_equal(response.correlation_id, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_error, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_message, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_request_id, AZ_SPAN_EMPTY));
  assert_true(az_span_is_content_equal(response.info_operation_expires, AZ_SPAN_EMPTY));
  assert_int_equal(response.retry_after_seconds, 0);
}

// ---- Logging tests ----

static int _log_invoked_topic = 0;
static void _log_listener(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
      assert_memory_equal(
          az_span_ptr(test_certificate_signing_requests_accepted_topic_202),
          az_span_ptr(message),
          (size_t)az_span_size(message));
      _log_invoked_topic++;
      break;
    default:
      assert_true(false);
  }
}

static bool _should_write_any_mqtt(az_log_classification classification)
{
  switch (classification)
  {
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
    case AZ_LOG_MQTT_RECEIVED_PAYLOAD:
      return true;
    default:
      return false;
  }
}

static bool _should_write_mqtt_received_payload_only(az_log_classification classification)
{
  switch (classification)
  {
    case AZ_LOG_MQTT_RECEIVED_PAYLOAD:
      return true;
    default:
      return false;
  }
}

static void test_az_iot_hub_client_certificate_signing_request_logging_succeed(void** state)
{
  (void)state;

  az_log_set_message_callback(_log_listener);
  az_log_set_classification_filter_callback(_should_write_any_mqtt);

  _log_invoked_topic = 0;

  az_iot_hub_client client;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;
  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certificate_signing_requests_accepted_topic_202, &response_info),
      AZ_OK);

  assert_int_equal(_az_BUILT_WITH_LOGGING(1, 0), _log_invoked_topic);

  az_log_set_message_callback(NULL);
  az_log_set_classification_filter_callback(NULL);
}

static void test_az_iot_hub_client_certificate_signing_request_no_logging_succeed(void** state)
{
  (void)state;

  az_log_set_message_callback(_log_listener);
  az_log_set_classification_filter_callback(_should_write_mqtt_received_payload_only);

  _log_invoked_topic = 0;

  az_iot_hub_client client;
  assert_true(
      az_iot_hub_client_init(&client, test_device_hostname, test_device_id, NULL) == AZ_OK);

  az_iot_hub_client_certificate_signing_response_info response_info;
  assert_int_equal(
      az_iot_hub_client_certificate_signing_request_parse_received_topic(
          &client, test_certificate_signing_requests_accepted_topic_202, &response_info),
      AZ_OK);

  assert_int_equal(_az_BUILT_WITH_LOGGING(0, 0), _log_invoked_topic);

  az_log_set_message_callback(NULL);
  az_log_set_classification_filter_callback(NULL);
}

// ---- Test runner ----

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_hub_client_certificate_signing_request()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_publish_topic_NULL_client_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_publish_topic_NULL_request_id_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_publish_topic_NULL_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_publish_topic_zero_size_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_parse_received_topic_NULL_client_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_parse_received_topic_NULL_topic_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_parse_received_topic_NULL_response_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_request_payload_NULL_client_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_request_payload_NULL_options_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_request_payload_NULL_buffer_fails),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_get_publish_topic_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_get_publish_topic_small_buffer_fails),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_publish_topic_exact_size_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_202_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_200_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_400_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_429_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_503_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_no_match_fails),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_received_topic_incomplete_fails),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_get_request_payload_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_get_request_payload_with_replace_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_get_request_payload_small_buffer_fails),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_accepted_response_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_parse_accepted_response_unknown_fields_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_error_response_400040_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_error_response_409005_succeed),
    cmocka_unit_test(
        test_az_iot_hub_client_certificate_signing_request_parse_error_response_retry_after_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_parse_error_response_no_info_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_logging_succeed),
    cmocka_unit_test(test_az_iot_hub_client_certificate_signing_request_no_logging_succeed),
  };

  return cmocka_run_group_tests_name("az_iot_hub_client_csr", tests, NULL, NULL);
}
