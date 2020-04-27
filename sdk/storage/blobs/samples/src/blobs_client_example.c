// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_context.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_transport.h>
#include <az_json.h>
#include <az_log.h>
#include <az_storage_blobs.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

#define URI_ENV "test_uri"

static az_span content_to_upload = AZ_SPAN_LITERAL_FROM_STR("Some test content");

// Uncomment below code to enable logging (and the first lines of main function)
/*
static void test_log_func(az_log_classification classification, az_span message)
{
  (void)classification;
  printf("%.*s\n", az_span_size(message), az_span_ptr(message));
}
*/

int main()
{
  // Uncomment below code to enable logging
  /*
  az_log_classification const classifications[] = { AZ_LOG_HTTP_RESPONSE, AZ_LOG_END_OF_LIST };
  az_log_set_classifications(classifications);
  az_log_set_callback(test_log_func);
  */

  // Init client.
  //  Example expects the URI_ENV to be a URL w/ SAS token
  az_storage_blobs_blob_client client = { 0 };
  az_storage_blobs_blob_client_options options = az_storage_blobs_blob_client_options_default();
  az_result const operation_result = az_storage_blobs_blob_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), AZ_CREDENTIAL_ANONYMOUS, &options);

  if (az_failed(operation_result))
  {
    printf("Failed to init blob client");
    return 1;
  }

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  az_result const init_http_response_result
      = az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer));

  if (az_failed(init_http_response_result))
  {
    printf("Failed to init http response");
    return 2;
  }

  printf("Uploading blob...\n");
  az_result const create_result = az_storage_blobs_blob_upload(
      &client, &az_context_app, content_to_upload, NULL, &http_response);

  // validate sample running with no_op http client
  if (create_result == AZ_ERROR_NOT_IMPLEMENTED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DBUILD_CURL_TRANSPORT=ON ..\n\n");

    return 3;
  }

  if (az_failed(create_result))
  {
    printf("Failed to create blob");
    return 4;
  }

  return 0;
}
