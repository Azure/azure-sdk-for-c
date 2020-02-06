// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_log.h>
#include <az_log_internal.h>
#include <az_log_private.h>
#include <az_str.h>

#include <az_test.h>

#include <_az_cfg.h>

static bool _log_invoked_for_http_request = false;
static bool _log_invoked_for_http_response = false;

static inline void _reset_log_invocation_status() {
  _log_invoked_for_http_request = false;
  _log_invoked_for_http_response = false;
}

static void _log_listener(az_log_classification const classification, az_span message) {
  // fprintf(stderr, "%.*s\n", (unsigned int)message.size, message.begin);
  switch (classification) {
    case AZ_LOG_HTTP_REQUEST:
      _log_invoked_for_http_request = true;
      TEST_ASSERT(az_span_is_equal(
          message,
          AZ_STR("HTTP Request : GET https://www.example.com\n"
                 "\t\tHeader1 : Value1\n"
                 "\t\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                 "\t\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc")));
      break;
    case AZ_LOG_HTTP_RESPONSE:
      _log_invoked_for_http_response = true;
      TEST_ASSERT(az_span_is_equal(
          message,
          AZ_STR("HTTP Response (3456ms) : 404 Not Found\n"
                 "\t\tHeader11 : Value11\n"
                 "\t\tHeader22 : NNNNOOOOPPPPQQQQRRRRSS ... UUUVVVVWWWWXXXXYYYYZZZZ\n"
                 "\t\tHeader33\n"
                 "\t\tHeader44 : cba8888887777776666665 ... 44444333333222222111111\n"
                 "\n"
                 "\tHTTP Request : GET https://www.example.com\n"
                 "\t\t\tHeader1 : Value1\n"
                 "\t\t\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                 "\t\t\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc")));
      break;
    default:
      TEST_ASSERT(false);
      break;
  }
}

void test_log() {
  // Set up test values etc.
  uint8_t hrb_buf[4 * 1024] = { 0 };
  az_http_request_builder hrb = { 0 };
  TEST_EXPECT_SUCCESS(az_http_request_builder_init(
      &hrb,
      (az_mut_span)AZ_SPAN_FROM_ARRAY(hrb_buf),
      200,
      AZ_HTTP_METHOD_VERB_GET,
      AZ_STR("https://www.example.com"),
      AZ_STR("AAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHHIIIIIJJJJJKKKKK")));

  TEST_EXPECT_SUCCESS(
      az_http_request_builder_append_header(&hrb, AZ_STR("Header1"), AZ_STR("Value1")));

  TEST_EXPECT_SUCCESS(az_http_request_builder_append_header(
      &hrb, AZ_STR("Header2"), AZ_STR("ZZZZYYYYXXXXWWWWVVVVUUUUTTTTSSSSRRRRQQQQPPPPOOOONNNN")));

  TEST_EXPECT_SUCCESS(az_http_request_builder_mark_retry_headers_start(&hrb));

  TEST_EXPECT_SUCCESS(az_http_request_builder_append_header(
      &hrb, AZ_STR("Header3"), AZ_STR("111111222222333333444444555555666666777777888888abc")));

  uint8_t response_buf[1024] = { 0 };
  az_span_builder response_builder
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(response_buf));

  TEST_EXPECT_SUCCESS(az_span_builder_append(
      &response_builder,
      AZ_STR("HTTP/1.1 404 Not Found\r\n"
             "Header11: Value11\r\n"
             "Header22: NNNNOOOOPPPPQQQQRRRRSSSSTTTTUUUUVVVVWWWWXXXXYYYYZZZZ\r\n"
             "Header33:\r\n"
             "Header44: cba888888777777666666555555444444333333222222111111\r\n"
             "\r\n"
             "KKKKKJJJJJIIIIIHHHHHGGGGGFFFFFEEEEEDDDDDCCCCCBBBBBAAAAA")));

  az_http_response response = { 0 };
  TEST_EXPECT_SUCCESS(az_http_response_init(&response, response_builder));
  // Finish setting up

  // Actual test below
  {
    // Verify that log callback gets invoked, and with the correct classification type.
    // Also, our callback function does the verification for the message content.
    _reset_log_invocation_status();
    az_log_set_listener(_log_listener);
    TEST_ASSERT(_log_invoked_for_http_request == false);
    TEST_ASSERT(_log_invoked_for_http_response == false);

    _az_log_http_request(&hrb);
    TEST_ASSERT(_log_invoked_for_http_request == true);
    TEST_ASSERT(_log_invoked_for_http_response == false);

    _az_log_http_response(&response, 3456, &hrb);
    TEST_ASSERT(_log_invoked_for_http_request == true);
    TEST_ASSERT(_log_invoked_for_http_response == true);
  }

  {
    _reset_log_invocation_status();
    az_log_set_listener(NULL);

    // Verify that user can unset log callback, and we are not going to call the previously set one.
    TEST_ASSERT(_log_invoked_for_http_request == false);
    TEST_ASSERT(_log_invoked_for_http_response == false);

    _az_log_http_request(&hrb);
    _az_log_http_response(&response, 3456, &hrb);

    TEST_ASSERT(_log_invoked_for_http_request == false);
    TEST_ASSERT(_log_invoked_for_http_response == false);

    {
      // Verify that our internal should_write() function would return false if noone is listening.
      TEST_ASSERT(az_log_should_write(AZ_LOG_HTTP_REQUEST) == false);
      TEST_ASSERT(az_log_should_write(AZ_LOG_HTTP_RESPONSE) == false);

      // If a callback is set, and no classificaions are specified, we are going to log all of them
      // (and customer is going to get all of them).
      az_log_set_listener(_log_listener);

      TEST_ASSERT(az_log_should_write(AZ_LOG_HTTP_REQUEST) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_HTTP_RESPONSE) == true);
    }

    // Verify that if customer specifies the classifications, we'll only invoking the logging
    // callback with the classification that's in the whitelist, and nothing is going to happen when
    // our code attempts to log a classification that's not in the customer's whitelist.
    az_log_classification const classifications[] = { AZ_LOG_HTTP_REQUEST };
    az_log_set_classifications(
        classifications, sizeof(classifications) / sizeof(classifications[0]));

    TEST_ASSERT(az_log_should_write(AZ_LOG_HTTP_REQUEST) == true);
    TEST_ASSERT(az_log_should_write(AZ_LOG_HTTP_RESPONSE) == false);

    _az_log_http_request(&hrb);
    _az_log_http_response(&response, 3456, &hrb);

    TEST_ASSERT(_log_invoked_for_http_request == true);
    TEST_ASSERT(_log_invoked_for_http_response == false);
  }

  az_log_set_classifications(NULL, 0);
  az_log_set_listener(NULL);
}
