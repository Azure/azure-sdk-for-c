// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file blobs_client_example.c
 * @brief
 * Notes:
 *   This sample requires an Storage account and shared access signature enabled for it.
 *
 * What this sample does:
 * 1) Creates blob client using the url and shared access signature on it to set up client
 *
 * 2) Creates an HTTP Response. It will be used to hold service response
 *
 * 3) Upload blob
 *
 * 4) Get payload from response and parse it
 */

#include <az_context.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_log.h>
#include <az_storage_blobs.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

#define URI_ENV "AZURE_STORAGE_URL"

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

  // 1) Init client.
  // Example expects AZURE_STORAGE_URL in env to be a URL w/ SAS token
  az_storage_blobs_blob_client client = { 0 };
  az_storage_blobs_blob_client_options options = az_storage_blobs_blob_client_options_default();
  az_result const init_blob_client_result = az_storage_blobs_blob_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), AZ_CREDENTIAL_ANONYMOUS, &options);

  if (az_failed(init_blob_client_result))
  {
    printf("\nFailed to init blob client\n");
    return 1;
  }

  /******* 2) Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  az_result const init_http_response_result
      = az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer));

  if (az_failed(init_http_response_result))
  {
    printf("\nFailed to init http response\n");
    return 2;
  }

  // 3) upload content
  printf("Uploading blob...\n");
  az_result const blob_upload_result = az_storage_blobs_blob_upload(
      &client, &az_context_app, content_to_upload, NULL, &http_response);

  // validate sample running with no_op http client
  if (blob_upload_result == AZ_ERROR_NOT_IMPLEMENTED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DTRANSPORT_CURL=ON ..\n\n");
    return 6;
  }

  if (az_failed(blob_upload_result))
  {
    printf("\nFailed to upload blob\n");
    return 3;
  }

  // 4) get response and parse it
  az_http_response_status_line status_line = { 0 };
  az_result const get_status_line_result
      = az_http_response_get_status_line(&http_response, &status_line);

  if (az_failed(get_status_line_result))
  {
    printf("\nFailed to get status line\n");
    return 4;
  }

  printf("Status Code: %d\n", status_line.status_code);
  printf(
      "Phrase: %.*s\n",
      az_span_size(status_line.reason_phrase),
      az_span_ptr(status_line.reason_phrase));

  printf("\nHeaders:\n");
  // loop all headers from response
  while (true)
  {
    az_pair header = { 0 };
    az_result const get_header_result = az_http_response_get_next_header(&http_response, &header);
    if (get_header_result == AZ_ERROR_ITEM_NOT_FOUND)
    {
      break;
    }

    if (az_failed(get_header_result))
    {
      printf("\nFailed to get header\n");
      return 5;
    }
    printf(
        "\t%.*s : %.*s\n",
        az_span_size(header.key),
        az_span_ptr(header.key),
        az_span_size(header.value),
        az_span_ptr(header.value));
  }

  return 0;
}
