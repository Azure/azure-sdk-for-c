// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_context.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_json.h>
#include <az_log.h>
#include <az_storage_blobs.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

#define URI_ENV "test_uri"

int exit_code = 0;

int main()
{
  // Init client.
  //  Example expects the URI_ENV to be a URL w/ SAS token
  az_storage_blobs_blob_client client = { 0 };
  az_storage_blobs_blob_client_options options = az_storage_blobs_blob_client_options_default();
  az_result const operation_result = az_storage_blobs_blob_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), AZ_CREDENTIAL_ANONYMOUS, &options);

  if (az_failed(operation_result))
  {
    printf("Failed to init blob client");
  }

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  az_result const init_http_response_result
      = az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer));

  if (az_failed(init_http_response_result))
  {
    printf("Failed to init http response");
  }

  az_result const create_result = az_storage_blobs_blob_upload(
      &client,
      &az_context_app,
      AZ_SPAN_FROM_STR("Some Test Content for the new blob"),
      NULL,
      &http_response);

  // validate sample running with no_op http client
  if (create_result == AZ_ERROR_NOT_IMPLEMENTED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DBUILD_CURL_TRANSPORT=ON ..\n\n");
    return 0;
  }

  if (az_failed(create_result))
  {
    printf("Failed to create blob");
  }

  return exit_code;
}
