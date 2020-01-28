// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_identity_client_secret_credential.h>
#include <az_json_get.h>
#include <az_storage_blobs.h>

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
AZ_NODISCARD az_result az_storage_blobs_blob_download(
    az_storage_blobs_blob_client * client,
    az_http_response * const response) {

  uint8_t request_buffer[1024 * 4] = { 0 };
  az_span request_buffer_span = AZ_SPAN_FROM_BUFFER(request_buffer);

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb, request_buffer_span, 1280, AZ_HTTP_METHOD_VERB_GET, client->uri, az_span_null()));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb, AZ_SPAN_FROM_STR("x-ms-version"), AZ_STORAGE_BLOBS_BLOB_API_VERSION));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

AZ_NODISCARD az_result az_storage_blobs_blob_delete(
    az_storage_blobs_blob_client * client,
    az_http_response * const response) {
  // Request buffer
  uint8_t request_buffer[1024 * 4] = { 0 };
  az_span request_buffer_span = AZ_SPAN_FROM_BUFFER(request_buffer);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb, request_buffer_span, 1280, AZ_HTTP_METHOD_VERB_DELETE, client->uri, az_span_null()));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb, AZ_SPAN_FROM_STR("x-ms-version"), AZ_STORAGE_BLOBS_BLOB_API_VERSION));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

int main() {

  az_storage_blobs_blob_client client;

  /************* create credentials as client_id type   ***********/
  az_identity_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result const creds_retcode = az_identity_client_secret_credential_init(
      &credential,
      az_span_from_str(getenv(TENANT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_SECRET_ENV)));

  if (az_failed(creds_retcode)) {
    printf("Failed to init credential");
  }

  // Init client.
  az_result const operation_result = az_storage_blobs_blob_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), &credential, NULL);

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
