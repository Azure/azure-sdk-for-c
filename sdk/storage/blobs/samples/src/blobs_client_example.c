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
static az_span content_to_upload = AZ_SPAN_LITERAL_FROM_STR("Some test Content");

/*
* uncomment this to enable logging
{
  (void)classification;
  printf("%.*s\n", az_span_length(message), az_span_ptr(message));
}
 */

/**
 * @brief Returns blob content in buffer
 *
 * @param client Client
 * @param response Response
 * @return AZ_NODISCARD az_storage_blobs_blob_download
 */
static AZ_NODISCARD az_result
az_storage_blobs_blob_download(az_storage_blobs_blob_client* client, az_http_response* response)
{

  // Request buffer
  // create request buffer TODO: define size for a getKey Request
  uint8_t url_buffer[1024 * 4];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(request_url_span, az_span_length(client->_internal.uri));
  request_url_span = az_span_append(request_url_span, client->_internal.uri);

  uint8_t headers_buffer[4 * sizeof(az_pair)];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  // TODO: define max URL size
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb,
      &az_context_app,
      az_http_method_get(),
      request_url_span,
      request_headers_span,
      AZ_SPAN_NULL));

  // start pipeline
  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&client->_internal.pipeline, &hrb, response));

  az_span body = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_body(response, &body));

  // do anything with the body, like print it out maybe
  printf("%.*s\n", az_span_length(body), az_span_ptr(body));

  return AZ_OK;
}

static AZ_NODISCARD az_result
az_storage_blobs_blob_delete(az_storage_blobs_blob_client* client, az_http_response* response)
{

  // Request buffer
  // create request buffer TODO: define size for blob delete
  uint8_t url_buffer[1024 * 4];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(request_url_span, az_span_length(client->_internal.uri));
  request_url_span = az_span_append(request_url_span, client->_internal.uri);

  uint8_t headers_buffer[4 * sizeof(az_pair)];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  // TODO: define max URL size
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb,
      &az_context_app,
      az_http_method_delete(),
      request_url_span,
      request_headers_span,
      AZ_SPAN_NULL));

  // start pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &hrb, response);
}

int main()
{
  /* Uncomment this to enable logging all http responses
  az_log_classification const classifications[] = { AZ_LOG_HTTP_RESPONSE, AZ_LOG_END_OF_LIST };
  az_log_set_classifications(classifications);
  az_log_set_callback(test_log_func); */

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

  printf("Uploading blob...\n");
  az_result const create_result = az_storage_blobs_blob_upload(
      &client, &az_context_app, content_to_upload, NULL, &http_response);

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

  printf("Downloading blob and printing it below:\n\n");
  az_result const get_result = az_storage_blobs_blob_download(&client, &http_response);

  if (az_failed(get_result))
  {
    printf("Failed to get blob");
  }

  printf("\nDeleting blob\n");
  az_result const delete_result = az_storage_blobs_blob_delete(&client, &http_response);

  if (az_failed(delete_result))
  {
    printf("Failed to delete blob");
  }

  printf("Sample finished\n");
  return exit_code;
}
