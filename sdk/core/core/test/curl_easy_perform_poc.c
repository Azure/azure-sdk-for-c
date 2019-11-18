// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_client_secret_credential.h>
#include <az_http_request_builder.h>
#include <az_http_response_parser.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_span_malloc.h>

#include <az_http_pipeline.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

static az_span KEY_VAULT_URL
    = AZ_CONST_STR("https://antk-keyvault.vault.azure.net/secrets/Password");

static az_span API_VERSION_QUERY_NAME = AZ_CONST_STR("api-version");
static az_span API_VERSION_QUERY_VALUE = AZ_CONST_STR("7.0");

int main() {
  // create a buffer for request
  uint8_t buf[1024 * 4];
  az_mut_span const http_buf = AZ_SPAN_FROM_ARRAY(buf);
  az_http_request_builder hrb;

  // response buffer
  uint8_t buf_response[1024 * 4];
  az_mut_span const http_buf_response = AZ_SPAN_FROM_ARRAY(buf_response);

  // create request for keyVault
  az_result build_result
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

  // Add auth
  az_client_secret_credential credentials = { 0 };
  uint8_t const * TENANT_ID = getenv("tenant_id");
  uint8_t const * CLIENT_ID = getenv("client_id");
  uint8_t const * CLIENT_SECRET = getenv("client_secret");
  az_result const creds_retcode = az_client_secret_credential_init(
      &credentials,
      (az_span){ .begin = TENANT_ID, .size = strlen(TENANT_ID) },
      (az_span){ .begin = CLIENT_ID, .size = strlen(CLIENT_ID) },
      (az_span){ .begin = CLIENT_SECRET, .size = strlen(CLIENT_SECRET) });

  if (!az_succeeded(creds_retcode)) {
    printf("Error initializing credentials\n");
    return creds_retcode;
  }

  az_http_policies policies = { 0 };
  az_result policies_retcode = az_http_policies_init(&policies);
  if (!az_succeeded(policies_retcode)) {
    printf("Error initializing policies\n");
    return policies_retcode;
  }

  policies.authentication.data = &credentials;

  // *************************launch pipeline
  az_result const get_response = az_http_pipeline_process(&hrb, &http_buf_response, &policies);

  if (az_succeeded(get_response)) {
    printf("Response is: \n%s", http_buf_response.begin);
  } else {
    printf("Error during running test\n");
  }

  return 0;
}
