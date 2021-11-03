// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file blobs_client_sample.c
 * @brief
 * Notes:
 *   This sample requires an Azure Storage account and shared access signature enabled for it.
 *
 * What this sample does:
 * 1) Creates blob client using the url and its shared access signature to set up client
 *
 * 2) Create an HTTP Response, which will be used to hold the service response
 *
 * 3) Upload the blob
 *
 * 4) Get payload from response and parse it
 *
 * 5) Download a blob
 *
 * 6) Get payload from response and parse it
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TRANSPORT_CURL
#include <curl/curl.h>
#endif

#include <azure/core/az_log.h>
#include <azure/storage/az_storage_blobs.h>

#define URI_ENV "AZURE_BLOB_URL_WITH_SAS"

static az_span content_to_upload = AZ_SPAN_LITERAL_FROM_STR("Some test content");

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

// Enable logging
void write_log_message(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_HTTP_REQUEST:
    case AZ_LOG_HTTP_RESPONSE:
      printf("%.*s\n", az_span_size(message), az_span_ptr(message));
  }
}

static bool print_http_response_headers(az_http_response* http_response);

int main()
{
#ifdef TRANSPORT_CURL
  // If running with libcurl, call global init. See project Readme for more info
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
  {
    printf("\nCouldn't init libcurl\n");
    return 1;
  }
  // Set up libcurl cleaning callback as to be called before ending program
  atexit(curl_global_cleanup);
#endif

  // Uncomment the line below to enable logging
  //az_log_set_message_callback(write_log_message);

  // 1) Init client.
  // Example expects AZURE_BLOB_URL_WITH_SAS in env to be a URL w/ SAS token
  az_storage_blobs_blob_client client = { 0 };

  {
    char* const blob_url = getenv(URI_ENV);
    if (blob_url == NULL)
    {
      printf("\nBlob URL environment variable " URI_ENV " not set.\n");
      return 1;
    }

    if (az_result_failed(az_storage_blobs_blob_client_init(
            &client, az_span_create_from_str(blob_url), AZ_CREDENTIAL_ANONYMOUS, NULL)))
    {
      printf("\nFailed to init blob client\n");
      return 1;
    }
  }

  /******* 2) Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  if (az_result_failed(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))))
  {
    printf("\nFailed to init http response\n");
    return 1;
  }

  // 3) upload content
  printf("\n=== Uploading blob... ===\n\n");
  az_result const blob_upload_result
      = az_storage_blobs_blob_upload(&client, NULL, content_to_upload, NULL, &http_response);

  // This validation is only for the first time SDK client is used. API will return not implemented
  // if samples were built with no_http lib.
  if (blob_upload_result == AZ_ERROR_DEPENDENCY_NOT_PROVIDED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DTRANSPORT_CURL=ON ..\n\n");

    return 1;
  }
  else if (az_result_failed(blob_upload_result)) // Any other error would terminate sample
  {
    printf("\nFailed to upload blob\n");
    return 1;
  }

  // 4) get response and parse it
  az_http_status_code status_code = az_http_response_get_status_code(&http_response);
  printf("Status Code: %d\n", status_code);
  printf("\nHeaders:\n");
  if (!print_http_response_headers(&http_response))
  {
    printf("\nFailed to get header\n");
    return 1;
  }

  // 4) download content
  memset(response_buffer, 0, sizeof(response_buffer));
  memset(&http_response, 0, sizeof(http_response));
  if (az_result_failed(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))))
  {
    printf("\nFailed to init http response\n");
    return 1;
  }

  printf("\n\n=== Downloading blob... ===\n\n");

  if (az_result_failed(az_storage_blobs_blob_download(&client, NULL, NULL, &http_response)))
  {
    printf("\nFailed to download blob\n");
    return 1;
  }

  // 4) get response and parse it
  status_code = az_http_response_get_status_code(&http_response);
  printf("Status Code: %d\n", status_code);
  printf("\nHeaders:\n");
  if (!print_http_response_headers(&http_response))
  {
    printf("\nFailed to get header\n");
    return 1;
  }

  az_span http_response_body = { 0 };
  if (az_result_failed(az_http_response_get_body(&http_response, &http_response_body)))
  {
    printf("\nFailed to get response body\n");
    return 1;
  }

  printf(
      "\nHTTP response body (blob content): %.*s\n",
      az_span_size(http_response_body),
      az_span_ptr(http_response_body));

  return 0;
}

static bool print_http_response_headers(az_http_response* http_response)
{
  while (true)
  {
    az_span header_name = { 0 };
    az_span header_value = { 0 };
    az_result const header_get_result
        = az_http_response_get_next_header(http_response, &header_name, &header_value);

    if (header_get_result == AZ_ERROR_HTTP_END_OF_HEADERS)
    {
      break;
    }
    else if (az_result_failed(header_get_result))
    {
      return false;
    }

    printf(
        "\t%.*s : %.*s\n",
        az_span_size(header_name),
        az_span_ptr(header_name),
        az_span_size(header_value),
        az_span_ptr(header_value));
  }

  return true;
}
