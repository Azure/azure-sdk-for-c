// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>
#include <az_identity_client_secret_credential.h>
#include <az_json_get.h>
#include <az_storage_blobs.h>

#include <_az_cfg.h>

#define TENANT_ID_ENV "tenant_id"
#define CLIENT_ID_ENV "client_id"
#define CLIENT_SECRET_ENV "client_secret"
#define URI_ENV "test_uri"

int exit_code = 0;

/**
 * @brief Returns blob content in buffer
 *
 * @param client
 * @param content
 * @param options
 * @param response
 * @return AZ_NODISCARD az_storage_blobs_blob_download
 */
AZ_NODISCARD az_result az_storage_blobs_blob_download(
    az_storage_blobs_blob_client * client,
    az_http_response * const response) {

  uint8_t request_buffer[1024 * 4] = { 0 };
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      1280,
      AZ_HTTP_METHOD_VERB_GET,
      client->uri,
      az_span_create_empty()));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb, AZ_STR("x-ms-version"), AZ_STORAGE_BLOBS_BLOB_API_VERSION));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

AZ_NODISCARD az_result az_storage_blobs_blob_delete(
    az_storage_blobs_blob_client * client,
    az_http_response * const response) {
  // Request buffer
  uint8_t request_buffer[1024 * 4] = { 0 };
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      1280,
      AZ_HTTP_METHOD_VERB_DELETE,
      client->uri,
      az_span_create_empty()));

    // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb, AZ_STR("x-ms-version"), AZ_STORAGE_BLOBS_BLOB_API_VERSION));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

int main() {

  az_storage_blobs_blob_client client;

  /************* create credentials as client_id type   ***********/
  az_identity_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result creds_retcode = az_identity_client_secret_credential_init(
      &credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  // Init client.
  az_result operation_result = az_storage_blobs_blob_client_init(
      &client, az_str_to_span(getenv(URI_ENV)), &credential, NULL);

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  az_result init_http_response_result = az_http_response_init(
      &http_response, az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer)));

  az_result create_result = az_storage_blobs_blob_upload(
      &client, AZ_STR("Some Test Content for the new blob"), NULL, &http_response);

  az_result get_result = az_storage_blobs_blob_download(&client, &http_response);

  az_result delete_result = az_storage_blobs_blob_delete(&client, &http_response);

  return exit_code;
}
