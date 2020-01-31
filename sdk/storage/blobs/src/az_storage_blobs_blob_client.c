// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_json.h>
#include <az_storage_blobs.h>

#include <_az_cfg.h>

/**
 * @brief Example URL syntax, not all URLs will be in this format
 * Basic url is expected as :
 * [https://]{account_id}[.blob.core.windows.net]/{container}/{blob}{optional_query} URL token max
 * Len            Total [https://]                       = 8                 8 {account_id} = 24 32
 * // 3-24 lowercase only
 * [.blob.core.windows.net]         = 22               54  //TODO: Support soveriegn clouds
 * /{container}                     = 64              118  // 1 + 3-63, lowercase letters/numbers/-
 * (consecutive dash is invalid)
 * /{blob}                          = 1025           1143  // 1 + 1024
 * {optional_query}                 = 70          ** 1213 **
 */
enum { MAX_URL_SIZE = 1280 }; // Padded to 1280
enum {
  MAX_BODY_SIZE = 1024 * 10
}; // 10KB buffer  /*TODO, Adjust this to reasonable size for non-stream data.*/

az_storage_blobs_blob_client_options const AZ_STORAGE_BLOBS_BLOB_CLIENT_DEFAULT_OPTIONS
    = { .retry = {
            .max_retry = 5,
            .delay_in_ms = 1000,
        } };

AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client * client,
    az_span const content,
    az_storage_blobs_blob_upload_options * const options,
    az_http_response * const response) {
  (void)options;

  // Request buffer
  // create request buffer TODO: define size for a blob upload
  uint8_t url_buffer[1024 * 4];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  uint8_t headers_buffer[4 * sizeof(az_pair)];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  // TODO: define max URL size
  az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb, AZ_HTTP_METHOD_GET, request_url_span, request_headers_span, content));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      &hrb, AZ_HTTP_HEADER_X_MS_VERSION, AZ_STORAGE_BLOBS_BLOB_API_VERSION));

  // add blob type to request
  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      &hrb, AZ_STORAGE_BLOBS_BLOB_HEADER_X_MS_BLOB_TYPE, client->blob_type));

  // add date to request
  // AZ_RETURN_IF_FAILED(az_http_request_append_header(
  //    &hrb, AZ_HTTP_HEADER_X_MS_DATE, AZ_SPAN_FROM_STR("Fri, 03 Jan 2020 21:33:15 GMT")));

  char str[256] = { 0 };
  // TODO: remove snprintf
  snprintf(str, sizeof str, "%d", az_span_length(content));
  az_span content_length = az_span_from_str(str);

  // add Content-Length to request
  AZ_RETURN_IF_FAILED(
      az_http_request_append_header(&hrb, AZ_HTTP_HEADER_CONTENT_LENGTH, content_length));

  // add blob type to request
  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      &hrb, AZ_HTTP_HEADER_CONTENT_TYPE, AZ_SPAN_FROM_STR("text/plain")));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}
