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
#include <az_storage_blobs.h>

// Uncomment below lines when working with libcurl
// #include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>

#define URI_ENV "AZURE_STORAGE_URL"

static az_span content_to_upload = AZ_SPAN_LITERAL_FROM_STR("Some test content");

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

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
  // Uncomment below lines when working with libcurl
  /*
    // If running with libcurl, call global init. See project Readme for more info
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
    {
      printf("\nCouldn't init libcurl\n");
      return 1;
    }
    // Set up libcurl cleaning callback as to be called before ending program
    atexit(curl_global_cleanup);
  */

  // Uncomment below code to enable logging
  /*
  az_log_classification const classifications[] = { AZ_LOG_HTTP_RESPONSE, AZ_LOG_END_OF_LIST };
  az_log_set_classifications(classifications);
  az_log_set_callback(test_log_func);
  */

  // 1) Init client.
  // Example expects AZURE_STORAGE_URL in env to be a URL w/ SAS token
  az_storage_blobs_blob_client client;
  az_storage_blobs_blob_client_options options = az_storage_blobs_blob_client_options_default();

  if (az_storage_blobs_blob_client_init(
          &client, az_span_from_str(getenv(URI_ENV)), AZ_CREDENTIAL_ANONYMOUS, &options)
      != AZ_OK)
  {
    printf("\nFailed to init blob client\n");
    return 1;
  }

  /******* 2) Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response;
  if (az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer)) != AZ_OK)
  {
    printf("\nFailed to init http response\n");
    return 1;
  }

  // 3) upload content
  printf("Uploading blob...\n");
  az_result const blob_upload_result = az_storage_blobs_blob_upload(
      &client, &az_context_app, content_to_upload, NULL, &http_response);

  // This validation is only for the first time SDK client is used. API will return not implemented
  // if samples were built with no_http lib.
  if (blob_upload_result == AZ_ERROR_NOT_IMPLEMENTED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DTRANSPORT_CURL=ON ..\n\n");

    return 1;
  }
  else if (blob_upload_result != AZ_OK) // Any other error would terminate sample
  {
    printf("\nFailed to upload blob\n");
    return 1;
  }

  // 4) get response and parse it
  az_http_response_status_line status_line;

  if (az_http_response_get_status_line(&http_response, &status_line) != AZ_OK)
  {
    printf("\nFailed to get status line\n");
    return 1;
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
    az_pair header;
    az_result const header_get_result = az_http_response_get_next_header(&http_response, &header);
    if (header_get_result == AZ_ERROR_ITEM_NOT_FOUND)
    {
      break;
    }
    else if (header_get_result != AZ_OK)
    {
      printf("\nFailed to get header\n");
      return 1;
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
