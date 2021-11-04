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

#include <azure/storage/az_storage_blobs.h>

#define URI_ENV "AZURE_BLOB_URL_WITH_SAS"

static az_span content_to_upload = AZ_SPAN_LITERAL_FROM_STR("Blob contents");

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

int main()
{
#ifdef TRANSPORT_CURL
  // If running with libcurl, call global init. See project Readme for more info
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
  {
    printf("Couldn't init libcurl.\n");
    return 1;
  }
  // Set up libcurl cleaning callback as to be called before ending program
  atexit(curl_global_cleanup);
#endif

  // 1) Init client.
  // Example expects AZURE_BLOB_URL_WITH_SAS in env to be a URL w/ SAS token
  az_storage_blobs_blob_client client = { 0 };

  {
    char* const blob_url = getenv(URI_ENV);
    if (blob_url == NULL)
    {
      printf("Blob URL environment variable " URI_ENV " not set.\n");
      return 1;
    }

    if (az_result_failed(az_storage_blobs_blob_client_init(
            &client, az_span_create_from_str(blob_url), AZ_CREDENTIAL_ANONYMOUS, NULL)))
    {
      printf("Failed to init blob client.\n");
      return 1;
    }
  }

  /******* 2) Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  if (az_result_failed(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))))
  {
    printf("\nFailed to init http response.\n");
    return 1;
  }

  // 3) upload content
  printf("Uploading blob...\n");
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
    printf("\nFailed to upload blob.\n");
    return 1;
  }

  // 4) get response and parse it
  // See https://docs.microsoft.com/rest/api/storageservices/put-blob#status-code for status codes.
  az_http_status_code status_code = az_http_response_get_status_code(&http_response);
  printf(
      "Status Code: %d (%s)\n",
      status_code,
      status_code == AZ_HTTP_STATUS_CODE_CREATED ? "success" : "error");

  // 4) download content
  memset(response_buffer, 0, sizeof(response_buffer));
  memset(&http_response, 0, sizeof(http_response));
  if (az_result_failed(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))))
  {
    printf("Failed to init http response.\n");
    return 1;
  }

  printf("\nDownloading blob...\n");

  if (az_result_failed(az_storage_blobs_blob_download(&client, NULL, NULL, &http_response)))
  {
    printf("Failed to download blob.\n");
    return 1;
  }

  // 4) get response and parse it
  // See https://docs.microsoft.com/rest/api/storageservices/get-blob#status-code for status codes.
  status_code = az_http_response_get_status_code(&http_response);
  printf(
      "Status Code: %d (%s)\n",
      status_code,
      status_code == AZ_HTTP_STATUS_CODE_OK ? "success" : "error");

  az_span http_response_body = { 0 };
  if (az_result_failed(az_http_response_get_body(&http_response, &http_response_body)))
  {
    printf("Failed to get response body.\n");
    return 1;
  }

  printf(
      "HTTP response body: %.*s\n",
      az_span_size(http_response_body),
      az_span_ptr(http_response_body));

  return 0;
}
