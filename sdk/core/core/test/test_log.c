// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_log.h>
#include <az_log_internal.h>
#include <az_log_private.h>
#include <az_str.h>

#include <az_test.h>

#include <_az_cfg.h>

static bool _log_invoked_for_request = false;
static bool _log_invoked_for_response = false;
static bool _log_invoked_for_slow_response = false;
static bool _log_invoked_for_error = false;

static inline void _reset_log_invocation_status() {
  _log_invoked_for_request = false;
  _log_invoked_for_response = false;
  _log_invoked_for_slow_response = false;
  _log_invoked_for_error = false;
}

static void _log_listener(az_log_classification const classification, az_span const message) {
  switch (classification) {
    case AZ_LOG_REQUEST:
      _log_invoked_for_request = true;
      TEST_ASSERT(az_span_is_equal(
          message,
          AZ_STR("HTTP Request : \n"
                 "\tVerb: GET\n"
                 "\tURL: https://www.example.com\n"
                 "\tHeaders: \n"
                 "\t\tHeader1 : Value1\n"
                 "\t\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                 "\t\t-- Retry Headers --\n"
                 "\t\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc")));
      break;
    case AZ_LOG_RESPONSE:
      _log_invoked_for_response = true;
      TEST_ASSERT(az_span_is_equal(
          message,
          AZ_STR("HTTP Request : \n"
                 "\tVerb: GET\n"
                 "\tURL: https://www.example.com\n"
                 "\tHeaders: \n"
                 "\t\tHeader1 : Value1\n"
                 "\t\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                 "\t\t-- Retry Headers --\n"
                 "\t\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc\n"
                 "\n"
                 "HTTP Response : \n"
                 "\tStatus Code: 451\n"
                 "\tStatus Line: Fahrenheit\n"
                 "\tHeaders: \n"
                 "\t\tHeader11 : Value11\n"
                 "\t\tHeader22 : NNNNOOOOPPPPQQQQRRRRSS ... UUUVVVVWWWWXXXXYYYYZZZZ\n"
                 "\t\tHeader33 : \n"
                 "\t\tHeader44 : cba8888887777776666665 ... 44444333333222222111111\n"
                 "\tBody: KKKKKJJJJJIIIIIHHHHHGG ... EEEDDDDDCCCCCBBBBBAAAAA")));
      break;
    case AZ_LOG_SLOW_RESPONSE:
      _log_invoked_for_slow_response = true;
      TEST_ASSERT(az_span_is_equal(
          message,
          AZ_STR("HTTP Request : \n"
                 "\tVerb: GET\n"
                 "\tURL: https://www.example.com\n"
                 "\tHeaders: \n"
                 "\t\tHeader1 : Value1\n"
                 "\t\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                 "\t\t-- Retry Headers --\n"
                 "\t\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc\n"
                 "\n"
                 "HTTP Response : \n"
                 "\tStatus Code: 451\n"
                 "\tStatus Line: Fahrenheit\n"
                 "\tHeaders: \n"
                 "\t\tHeader11 : Value11\n"
                 "\t\tHeader22 : NNNNOOOOPPPPQQQQRRRRSS ... UUUVVVVWWWWXXXXYYYYZZZZ\n"
                 "\t\tHeader33 : \n"
                 "\t\tHeader44 : cba8888887777776666665 ... 44444333333222222111111\n"
                 "\tBody: KKKKKJJJJJIIIIIHHHHHGG ... EEEDDDDDCCCCCBBBBBAAAAA\n"
                 "\n"
                 "Response Duration: 3456ms")));
      break;
    case AZ_LOG_ERROR:
      _log_invoked_for_error = true;
      TEST_ASSERT(az_span_is_equal(
          message,
          AZ_STR("HTTP Request : \n"
                 "\tVerb: GET\n"
                 "\tURL: https://www.example.com\n"
                 "\tHeaders: \n"
                 "\t\tHeader1 : Value1\n"
                 "\t\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                 "\t\t-- Retry Headers --\n"
                 "\t\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc\n"
                 "\n"
                 "HTTP Response : \n"
                 "\tStatus Code: 451\n"
                 "\tStatus Line: Fahrenheit\n"
                 "\tHeaders: \n"
                 "\t\tHeader11 : Value11\n"
                 "\t\tHeader22 : NNNNOOOOPPPPQQQQRRRRSS ... UUUVVVVWWWWXXXXYYYYZZZZ\n"
                 "\t\tHeader33 : \n"
                 "\t\tHeader44 : cba8888887777776666665 ... 44444333333222222111111\n"
                 "\tBody: KKKKKJJJJJIIIIIHHHHHGG ... EEEDDDDDCCCCCBBBBBAAAAA\n"
                 "\n"
                 "Error: 2147680257")));
      break;
    default:
      TEST_ASSERT(false);
      break;
  }
}

void test_log() {
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
      AZ_STR("HTTP/1.1 451 Fahrenheit\r\n"
             "Header11: Value11\r\n"
             "Header22: NNNNOOOOPPPPQQQQRRRRSSSSTTTTUUUUVVVVWWWWXXXXYYYYZZZZ\r\n"
             "Header33:\r\n"
             "Header44: cba888888777777666666555555444444333333222222111111\r\n"
             "\r\n"
             "KKKKKJJJJJIIIIIHHHHHGGGGGFFFFFEEEEEDDDDDCCCCCBBBBBAAAAA")));

  az_http_response response = { 0 };
  TEST_EXPECT_SUCCESS(az_http_response_init(&response, response_builder));

  {
    _reset_log_invocation_status();
    az_log_set_listener(&_log_listener);
    TEST_ASSERT(_log_invoked_for_request == false);
    TEST_ASSERT(_log_invoked_for_response == false);
    TEST_ASSERT(_log_invoked_for_slow_response == false);
    TEST_ASSERT(_log_invoked_for_error == false);

    _az_log_request(&hrb);
    TEST_ASSERT(_log_invoked_for_request == true);
    TEST_ASSERT(_log_invoked_for_response == false);
    TEST_ASSERT(_log_invoked_for_slow_response == false);
    TEST_ASSERT(_log_invoked_for_error == false);

    _az_log_response(&hrb, &response);
    TEST_ASSERT(_log_invoked_for_request == true);
    TEST_ASSERT(_log_invoked_for_response == true);
    TEST_ASSERT(_log_invoked_for_slow_response == false);
    TEST_ASSERT(_log_invoked_for_error == false);

    _az_log_slow_response(&hrb, &response, 3456);
    TEST_ASSERT(_log_invoked_for_request == true);
    TEST_ASSERT(_log_invoked_for_response == true);
    TEST_ASSERT(_log_invoked_for_slow_response == true);
    TEST_ASSERT(_log_invoked_for_error == false);

    _az_log_error(&hrb, &response, AZ_ERROR_HTTP_PAL);
    TEST_ASSERT(_log_invoked_for_request == true);
    TEST_ASSERT(_log_invoked_for_response == true);
    TEST_ASSERT(_log_invoked_for_slow_response == true);
    TEST_ASSERT(_log_invoked_for_error == true);
  }

  {
    _reset_log_invocation_status();
    az_log_set_listener(NULL);

    TEST_ASSERT(_log_invoked_for_request == false);
    TEST_ASSERT(_log_invoked_for_response == false);
    TEST_ASSERT(_log_invoked_for_slow_response == false);
    TEST_ASSERT(_log_invoked_for_error == false);

    _az_log_request(&hrb);
    _az_log_response(&hrb, &response);
    _az_log_slow_response(&hrb, &response, 3456);
    _az_log_error(&hrb, &response, AZ_ERROR_HTTP_PAL);

    TEST_ASSERT(_log_invoked_for_request == false);
    TEST_ASSERT(_log_invoked_for_response == false);
    TEST_ASSERT(_log_invoked_for_slow_response == false);
    TEST_ASSERT(_log_invoked_for_error == false);

    {
      TEST_ASSERT(az_log_should_write(AZ_LOG_REQUEST) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_RESPONSE) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_SLOW_RESPONSE) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_ERROR) == true);

      az_log_set_listener(&_log_listener);

      TEST_ASSERT(az_log_should_write(AZ_LOG_REQUEST) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_RESPONSE) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_SLOW_RESPONSE) == true);
      TEST_ASSERT(az_log_should_write(AZ_LOG_ERROR) == true);
    }

    az_log_classification classifications[] = { AZ_LOG_ERROR, AZ_LOG_SLOW_RESPONSE };
    az_log_set_classifications(classifications, 2);

    TEST_ASSERT(az_log_should_write(AZ_LOG_REQUEST) == false);
    TEST_ASSERT(az_log_should_write(AZ_LOG_RESPONSE) == false);
    TEST_ASSERT(az_log_should_write(AZ_LOG_SLOW_RESPONSE) == true);
    TEST_ASSERT(az_log_should_write(AZ_LOG_ERROR) == true);

    _az_log_request(&hrb);
    _az_log_response(&hrb, &response);
    _az_log_slow_response(&hrb, &response, 3456);
    _az_log_error(&hrb, &response, AZ_ERROR_HTTP_PAL);

    TEST_ASSERT(_log_invoked_for_request == false);
    TEST_ASSERT(_log_invoked_for_response == false);
    TEST_ASSERT(_log_invoked_for_slow_response == true);
    TEST_ASSERT(_log_invoked_for_error == true);
  }

  az_log_set_classifications(NULL, 0);
  az_log_set_listener(NULL);
}
