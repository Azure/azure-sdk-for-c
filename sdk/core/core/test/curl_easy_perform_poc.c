// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_client.h>
#include <az_http_request_builder.h>
#include <az_json_read.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_span_builder.h>

#include <az_http_pipeline.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

static az_const_span GET_TOKEN_URL = AZ_CONST_STR(
    "https://login.microsoftonline.com/72f988bf-86f1-41af-91ab-2d7cd011db47/oauth2/token");
static az_const_span KEY_VAULT_URL
    = AZ_CONST_STR("https://antk-keyvault.vault.azure.net/secrets/Password");

static az_const_span API_VERSION_QUERY_NAME = AZ_CONST_STR("api-version");
static az_const_span API_VERSION_QUERY_VALUE = AZ_CONST_STR("7.0");

static az_const_span token_request_body
    = AZ_CONST_STR("grant_type=client_credentials&client_id=4317a660-6bfb-4585-9ce9-8f222314879c&"
             "client_secret=O2CT[Y:dkTqblml5V/T]ZEi9x1W1zoBW&resource=https://vault.azure.net");

int main() {
  // create a buffer for request
  uint8_t buf[1024 * 4];
  az_span const http_buf = AZ_SPAN(buf);
  az_http_request_builder hrb;

  // response buffer
  uint8_t buf_response[1024 * 4];
  az_span const http_buf_response = AZ_SPAN(buf_response);

  // init request with POST for getting a token
  az_result build_result
      = az_http_request_builder_init(&hrb, http_buf, 100, AZ_HTTP_METHOD_VERB_POST, GET_TOKEN_URL);
  if (az_failed(build_result)) {
    return build_result;
  }
  // attach a body for the POST request
  az_result const add_body_result = az_http_request_builder_add_body(&hrb, token_request_body);
  if (az_failed(add_body_result)) {
    return add_body_result;
  }
  // *************************send POST
  az_result const post_response = az_http_client_send_request(&hrb, &http_buf_response);

  if (az_failed(post_response)) {
    printf("Error during running test\n");
    return post_response;
  }

  // create request for keyVault
  build_result
      = az_http_request_builder_init(&hrb, http_buf, 100, AZ_HTTP_METHOD_VERB_GET, KEY_VAULT_URL);
  if (az_failed(build_result)) {
    return build_result;
  }

  // add query param
  az_result add_query_result = az_http_request_builder_set_query_parameter(
      &hrb, API_VERSION_QUERY_NAME, API_VERSION_QUERY_VALUE);
  if (az_failed(add_query_result)) {
    return add_query_result;
  }

  // build auth header
  az_result ignore_result;
  az_const_span token = { .begin = 0, .size = 0 };
  ignore_result = az_json_get_object_member_string_value(
      az_span_to_const_span(http_buf_response), AZ_STR("access_token"), &token);
  // printf("******* %s *****", token.begin);

  /****** -------------  Create buffer for header auth ---------******/
  // can't print token right now since it is not 0-terminated
  size_t const buffer_for_header_size = sizeof("Bearer ") + token.size;
  uint8_t * const buffer_for_header = (uint8_t *)malloc(buffer_for_header_size);

  /****** -------------  use Span builder to concatenate ---------******/
  az_span temp_buf = (az_span){ .begin = buffer_for_header, .size = buffer_for_header_size };
  az_span_builder builder = az_span_builder_create(temp_buf);
  ignore_result = az_span_builder_append(&builder, AZ_STR("Bearer "));
  ignore_result = az_span_builder_append(&builder, token);
  ignore_result = az_span_builder_append(
      &builder, AZ_STR_ZERO); // add a 0 so it can be printed and used by Curl

  // add auth Header with parsed token
  az_result const add_header_result = az_http_request_builder_append_header(
      &hrb, AZ_STR("authorization"), az_span_to_const_span(temp_buf));
  if (az_failed(add_header_result)) {
    return add_header_result;
  }

  // *************************send GET
  az_result const get_response = az_http_client_send_request(&hrb, &http_buf_response);

  if (az_succeeded(get_response)) {
    printf("Response is: \n%s", http_buf_response.begin);
  } else {
    printf("Error during running test\n");
    return get_response;
  }

  free(buffer_for_header);

  az_http_pipeline const pipeline;

  az_http_response_data const response;
  az_result result;
  result = az_http_pipeline_process(&pipeline, &request, &response);

  return 0;
}
