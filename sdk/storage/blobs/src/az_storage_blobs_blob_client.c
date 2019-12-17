// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>
#include <az_storage_blobs.h>

#include <_az_cfg.h>

/**
 * @brief Maximum allowed URL size:
 * url is expected as : [https://]{account_id}[.blob.core.windows.net]/{container}/{blob}{optional_query}
 * URL token                       max Len            Total
 * [https://]                       = 8                 8
 * {account_id}                     = 24               32  // 3-24 lowercase only
 * [.blob.core.windows.net]         = 22               54  //TODO: Support soveriegn clouds
 * /{container}                     = 63              117  // 3-63, lowercase letters/numbers/- (consecutive dash is invalid)
 * /{blob}                          = 1024           1141
 * {optional_query}                 = 70          ** 1211 **
 */
enum { MAX_URL_SIZE = 1211 };
enum { MAX_BODY_SIZE = 1024 * 10 }; // 10KB buffer  /*TODO, Adjust this to reasonable size for non-stream data.*/

AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_client_constant_date() { return AZ_STR("x-ms-date");}
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_client_constant_blob_type() { return AZ_STR("x-ms-blob-type");}
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_client_constant_content_length() { return AZ_STR("Content-Length");}
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_client_constant_content_type() { return AZ_STR("Content-Type");}
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_client_constant_text_plain() { return AZ_STR("text/plain; charset=UTF-8");}

az_storage_blobs_blob_client_options const AZ_STORAGE_BLOBS_BLOB_CLIENT_DEFAULT_OPTIONS
    = { .service_version = AZ_CONST_STR("2015-02-21"),
        .retry = {
            .max_retry = 3,
            .delay_in_ms = 30,
        } };

AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client * client,
    az_span const content,
    az_storage_blobs_blob_upload_options * const options,
    az_http_response * const response) {

  // Request buffer
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  /* ******** build url for request  ******/

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_PUT,
      client->uri,
      content));
  
  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->client_options.service_version));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

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
    az_span_builder content,
    az_storage_blobs_blob_download_options options,
    az_http_response * const response) {
  
    uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      client->uri,
      az_span_create_empty()));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

AZ_NODISCARD az_result az_storage_blobs_blob_delete(
    az_storage_blobs_blob_client * client,
    az_http_response * const response) {
  // Request buffer
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_DELETE,
      client->uri,
      az_span_create_empty()));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}
