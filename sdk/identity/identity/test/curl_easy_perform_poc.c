// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy_retry_options.h>
#include <az_http_request_builder.h>
#include <az_http_response_parser.h>
#include <az_identity_access_token_context.h>
#include <az_identity_client_secret_credential.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_span_malloc_internal.h>
#include <az_str.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

#define TENANT_ID_ENV "tenant_id"
#define CLIENT_ID_ENV "client_id"
#define CLIENT_SECRET_ENV "client_secret"
#define URI_ENV "test_uri"

static az_span const API_VERSION_QUERY_NAME = AZ_CONST_STR("api-version");
static az_span const API_VERSION_QUERY_VALUE = AZ_CONST_STR("7.0");

int main() {
  // create a buffer for request
  uint8_t buf[1024 * 4];
  az_mut_span const http_buf = AZ_SPAN_FROM_ARRAY(buf);
  az_http_request_builder hrb = { 0 };

  // response buffer
  uint8_t buf_response[1024 * 4];
  az_http_response http_buf_response
      = { .builder = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(buf_response)) };

  // create request for keyVault
  az_result build_result = az_http_request_builder_init(
      &hrb,
      http_buf,
      100,
      AZ_HTTP_METHOD_VERB_GET,
      az_str_to_span(getenv(URI_ENV)),
      az_span_empty());

  if (az_failed(build_result)) {
    return build_result;
  }

  // add query param
  az_result add_query_result = az_http_request_builder_set_query_parameter(
      &hrb, API_VERSION_QUERY_NAME, API_VERSION_QUERY_VALUE);
  if (az_failed(add_query_result)) {
    return add_query_result;
  }

  // Add auth
  az_identity_client_secret_credential credential = { 0 };
  az_result const creds_retcode = az_identity_client_secret_credential_init(
      &credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  if (!az_succeeded(creds_retcode)) {
    printf("Error initializing credential\n");
    return creds_retcode;
  }

  az_identity_access_token access_token = { 0 };
  az_result const token_retcode = az_identity_access_token_init(&access_token);

  if (!az_succeeded(token_retcode)) {
    printf("Error initializing access token\n");
    return token_retcode;
  }

  az_identity_access_token_context access_token_context = { 0 };
  az_result const token_context_retcode = az_identity_access_token_context_init(
      &access_token_context,
      &credential,
      &access_token,
      AZ_STR("https://vault.azure.net/.default"));

  if (!az_succeeded(token_retcode)) {
    printf("Error initializing access token context\n");
    return token_context_retcode;
  }

  az_http_policy_retry_options retry_options = az_http_policy_retry_options_create();
  az_http_pipeline pipeline = (az_http_pipeline){
      .policies = {
        { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_retry, .data = &retry_options },
        { .pfnc_process = az_http_pipeline_policy_authentication, .data = &access_token_context },
        { .pfnc_process = az_http_pipeline_policy_logging, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_transport, .data = NULL },
        { .pfnc_process = NULL, .data = NULL },
      }, 
    };

  // *************************launch pipeline
  az_result const get_response = az_http_pipeline_process(&pipeline, &hrb, &http_buf_response);

  if (az_succeeded(get_response)) {
    az_span const response = az_span_builder_result(&http_buf_response.builder);
    printf("Response is: \n%.*s", (unsigned int)response.size, response.begin);
  } else {
    printf("Error during running test\n");
  }

  return 0;
}
