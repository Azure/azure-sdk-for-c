// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file blobs_client_sample_multiblock.c
 * @brief
 * Notes:
 *   This sample requires an IoT Hub device created with X.509 certificate authentication and Azure Storage account enabled for this IoT Hub.
 *
 * What this sample does:
 * 1) Creates blob client using the url and its shared access signature to set up client
 *
 * 2) Creates an HTTP Response, which will be used to hold the service response
 *
 * 3) Uploads the block blob with 100 blocks using callback function 'get_data_callback()'
 *
 * 4) Get payload from response and parse it
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

static int sample_show_http_response(az_http_response *http_response);
az_result get_data_callback(az_span *data_block);

static int  block_count = 0;
static char data_to_upload[256] = { 0 };

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

static int sample_show_http_response(az_http_response *http_response)
{
  az_http_status_code status_code = az_http_response_get_status_code(http_response);
  printf("Status Code: %d\n", status_code);

  az_span http_response_body = { 0 };
  if (az_result_failed(az_http_response_get_body(http_response, &http_response_body)))
  {
    printf("Failed to get response body\n\n");
    return -1;
  }

  printf(
        "HTTP response body: \"%.*s\"\n\n",
        az_span_size(http_response_body),
        az_span_ptr(http_response_body));

  return 0;
}

az_result get_data_callback(az_span *data_block)
{
  if (block_count < 100)
  {
    int len = snprintf(data_to_upload, sizeof(data_to_upload), 
              "az_storage_blobs_multiblock_blob_upload block: %d\n", block_count);
    if (len > 0)
    {
      if ((size_t)len >= sizeof(data_to_upload))
      {
          block_count = 0;
          return AZ_ERROR_NOT_ENOUGH_SPACE;
      }

      *data_block = az_span_create_from_str(data_to_upload);
      block_count++;
      printf("Sending blob block No. %d...\n", block_count);
    }
    else
    {
      *data_block = AZ_SPAN_EMPTY;
      block_count = 0;
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
  }
  else
  {
    *data_block = AZ_SPAN_EMPTY;
    block_count = 0;
  }

  return AZ_OK;
}

int main()
{
#ifdef TRANSPORT_CURL
  // If running with libcurl, call global init. See project Readme for more info
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
  {
    printf("Couldn't init libcurl.\n");
    return 1;
  }

  // Set up libcurl cleaning callback as to  be called before ending program
  atexit(curl_global_cleanup);
#endif

  // 1) Init client
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

  // 2) Create multiblock blob in the storage container and send data
  //      using the function get_data_callback
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  if (az_result_failed(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))))
  {
     printf("\nFailed to init http response.\n");
    return 1;
  }

  printf("Sending 100 blocks of data to the block blob...\n");
  az_result const multiblock_blob_upload_result = az_storage_blobs_multiblock_blob_upload(
      &client, NULL, get_data_callback, NULL, &http_response);
                    
  if (az_result_failed(multiblock_blob_upload_result))
  {
    sample_show_http_response(&http_response);
    printf("az_storage_blobs_multiblock_upload failed. ret = 0x%02x\n", multiblock_blob_upload_result);
    return 1;
  }
  
  if (az_http_response_get_status_code(&http_response) != AZ_HTTP_STATUS_CODE_CREATED)
  {
    sample_show_http_response(&http_response);
    printf("Blocks have not been uploaded.\n");
    return 1;
  }

  sample_show_http_response(&http_response);
  printf("Blocks have been successfully uploaded to the block blob.\n\n");

  return 0;
}
