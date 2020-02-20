// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_result.h>
#include <az_span.h>

#include <az_curl.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./az_test.h"

#include <_az_cfg.h>

int exit_code = 0;

void get_request_for_OK_expected();
void post_request_for_OK_expected();
void get_request_overflow_response();
void get_request_unresolved_host();

int main() {
  get_request_for_OK_expected();
  post_request_for_OK_expected();
  get_request_overflow_response();
  get_request_unresolved_host();
  
  return exit_code;
}

void get_request_for_OK_expected() {
  // send a basic GET request and expect AZ_OK result
  az_span sample_url
      = AZ_SPAN_FROM_STR("https://raw.githubusercontent.com/Azure/azure-rest-api-specs/master/"
                         "specification/keyvault/resource-manager/readme.md");

  // Response buffer
  uint8_t response_buffer[1024 * 6];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);

  az_http_response sample_response;
  az_result init_response_result = az_http_response_init(&sample_response, response_span);

  TEST_ASSERT(init_response_result == AZ_OK);

  _az_http_request sample_request;
  az_result init_request_result = az_http_request_init(
      &sample_request, az_http_method_get(), sample_url, AZ_SPAN_NULL, AZ_SPAN_NULL);

  TEST_ASSERT(init_request_result == AZ_OK);

  az_http_transport_options transporter
      = az_http_transport_options_default(_az_http_client_curl_send_request);

  // send request
  az_result send_req_result = transporter._internal.send_request(&sample_request, &sample_response);

  TEST_ASSERT(send_req_result == AZ_OK);

  az_http_response_status_line status_line;
  TEST_ASSERT(az_http_response_get_status_line(&sample_response, &status_line) == AZ_OK);

  TEST_ASSERT(status_line.status_code == 200);
}

void post_request_for_OK_expected() {
  // send a basic POST request and expect AZ_OK result with 403 Forbidden response
  az_span sample_url = AZ_SPAN_FROM_STR("https://www.microsoft.com/");

  // Response buffer
  uint8_t response_buffer[1024 * 5];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);

  az_http_response sample_response;
  az_result init_response_result = az_http_response_init(&sample_response, response_span);

  TEST_ASSERT(init_response_result == AZ_OK);

  _az_http_request sample_request;
  az_result init_request_result = az_http_request_init(
      &sample_request, az_http_method_post(), sample_url, AZ_SPAN_NULL, AZ_SPAN_NULL);

  TEST_ASSERT(init_request_result == AZ_OK);

  az_http_transport_options transporter
      = az_http_transport_options_default(_az_http_client_curl_send_request);

  // send request
  az_result send_req_result = transporter._internal.send_request(&sample_request, &sample_response);

  TEST_ASSERT(send_req_result == AZ_OK);

  az_http_response_status_line status_line;
  TEST_ASSERT(az_http_response_get_status_line(&sample_response, &status_line) == AZ_OK);

  TEST_ASSERT(status_line.status_code == 403);
}

void get_request_overflow_response() {
  // send a basic GET request and expect OVERFLOW (Response too big for response buffer)
  az_span sample_url = AZ_SPAN_FROM_STR("https://support.microsoft.com/en-us/contactus/");

  // Response buffer
  uint8_t response_buffer[1024 * 6];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);

  az_http_response sample_response;
  az_result init_response_result = az_http_response_init(&sample_response, response_span);

  TEST_ASSERT(init_response_result == AZ_OK);

  _az_http_request sample_request;
  az_result init_request_result = az_http_request_init(
      &sample_request, az_http_method_get(), sample_url, AZ_SPAN_NULL, AZ_SPAN_NULL);

  TEST_ASSERT(init_request_result == AZ_OK);

  az_http_transport_options transporter
      = az_http_transport_options_default(_az_http_client_curl_send_request);

  // send request
  az_result send_req_result = transporter._internal.send_request(&sample_request, &sample_response);

  TEST_ASSERT(send_req_result == AZ_ERROR_HTTP_RESPONSE_OVERFLOW);
}

void get_request_unresolved_host() {
  // send a basic GET request and expect HOST not resolved error
  az_span sample_url = AZ_SPAN_FROM_STR("https://noServerForNoResolvedResponse/");

  // Response buffer
  uint8_t response_buffer[1024 * 5];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);

  az_http_response sample_response;
  az_result init_response_result = az_http_response_init(&sample_response, response_span);

  TEST_ASSERT(init_response_result == AZ_OK);

  _az_http_request sample_request;
  az_result init_request_result = az_http_request_init(
      &sample_request, az_http_method_get(), sample_url, AZ_SPAN_NULL, AZ_SPAN_NULL);

  TEST_ASSERT(init_request_result == AZ_OK);

  az_http_transport_options transporter
      = az_http_transport_options_default(_az_http_client_curl_send_request);

  // send request
  az_result send_req_result = transporter._internal.send_request(&sample_request, &sample_response);

  TEST_ASSERT(send_req_result == AZ_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST);
}
