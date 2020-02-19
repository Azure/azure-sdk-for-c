// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_credentials.h>
#include <az_curl.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_json.h>
#include <az_storage_blobs.h>
#include <az_log.h>

#include <stdio.h>
#include <stdlib.h>

#include <_az_cfg.h>

#define TENANT_ID_ENV "tenant_id"
#define CLIENT_ID_ENV "client_id"
#define CLIENT_SECRET_ENV "client_secret"
#define URI_ENV "test_uri"

int exit_code = 0;

/**
 * @brief Returns blob content in buffer
 *
 * @param client Client
 * @param response Response
 * @return AZ_NODISCARD az_storage_blobs_blob_download
 */
AZ_NODISCARD az_result
az_storage_blobs_blob_download(az_storage_blobs_blob_client * client, az_http_response * response) {

  // Request buffer
  // create request buffer TODO: define size for a getKey Request
  uint8_t url_buffer[1024 * 4];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  AZ_RETURN_IF_FAILED(az_span_append(request_url_span, client->_internal.uri, &request_url_span));

  uint8_t headers_buffer[4 * sizeof(az_pair)];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  // TODO: define max URL size
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb, az_http_method_get(), request_url_span, request_headers_span, az_span_null()));

  // start pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &hrb, response);
}

AZ_NODISCARD az_result
az_storage_blobs_blob_delete(az_storage_blobs_blob_client * client, az_http_response * response) {

  // Request buffer
  // create request buffer TODO: define size for blob delete
  uint8_t url_buffer[1024 * 4];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  AZ_RETURN_IF_FAILED(az_span_append(request_url_span, client->_internal.uri, &request_url_span));

  uint8_t headers_buffer[4 * sizeof(az_pair)];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  // TODO: define max URL size
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb, az_http_method_delete(), request_url_span, request_headers_span, az_span_null()));

  // start pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &hrb, response);
}

int main() {

  az_storage_blobs_blob_client client;

  /*  */
  az_http_transport_options http_transport_options
      = az_http_transport_options_default(_az_http_client_curl_send_request);

  /************* create credentials as client_id type   ***********/
  az_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result const creds_retcode = az_client_secret_credential_init(
      &credential,
      az_span_from_str(getenv(TENANT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_SECRET_ENV)),
      &http_transport_options
     );

  if (az_failed(creds_retcode)) {
    printf("Failed to init credential");
  }

  // Init client.
  az_storage_blobs_blob_client_options options
      = az_storage_blobs_blob_client_options_default(&http_transport_options);

  // Init client.
  az_result const operation_result = az_storage_blobs_blob_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), &credential, &options);

  if (az_failed(operation_result)) {
    printf("Failed to init blob client");
  }

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  az_result const init_http_response_result
      = az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer));

  if (az_failed(init_http_response_result)) {
    printf("Failed to init http response");
  }

  az_result const create_result = az_storage_blobs_blob_upload(
      &client, AZ_SPAN_FROM_STR("Some Test Content for the new blob"), NULL, &http_response);

  if (az_failed(create_result)) {
    printf("Failed to create blob");
  }

  az_result const get_result = az_storage_blobs_blob_download(&client, &http_response);

  if (az_failed(get_result)) {
    printf("Failed to get blob");
  }

  az_result const delete_result = az_storage_blobs_blob_delete(&client, &http_response);

  if (az_failed(delete_result)) {
    printf("Failed to delete blob");
  }

  return exit_code;
}
