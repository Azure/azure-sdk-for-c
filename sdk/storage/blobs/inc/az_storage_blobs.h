// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_STORAGE_BLOBS_H
#define _az_STORAGE_BLOBS_H

#include <az_config.h>
#include <az_context.h>
#include <az_contract_internal.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

static az_span const AZ_STORAGE_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("2019-02-02");

typedef struct
{
  az_http_policy_retry_options retry;
  /// @internal <a></a>
  struct
  {
    _az_http_policy_apiversion_options api_version;
    _az_http_policy_telemetry_options _telemetry_options;
  } _internal;
  /// @endinternal <a></a>
} az_storage_blobs_blob_client_options;

typedef struct
{
  /// @internal <a></a>
  struct
  {
    // buffer to copy customer url. Then it stays immutable
    uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUF_SIZE];
    // this url will point to url_buffer
    az_span uri;
    _az_http_pipeline pipeline;
    az_storage_blobs_blob_client_options options;
    _az_credential* credential;
  } _internal;
  /// @endinternal <a></a>
} az_storage_blobs_blob_client;

AZ_NODISCARD az_result az_storage_blobs_blob_client_init(
    az_storage_blobs_blob_client* client,
    az_span uri,
    void* credential,
    az_storage_blobs_blob_client_options* options);

typedef struct
{
  az_span option;
} az_storage_blobs_blob_upload_options;

AZ_NODISCARD az_storage_blobs_blob_client_options az_storage_blobs_blob_client_options_default();

AZ_NODISCARD AZ_INLINE az_storage_blobs_blob_upload_options
az_storage_blobs_blob_upload_options_default()
{
  return (az_storage_blobs_blob_upload_options){ .option = AZ_SPAN_NULL };
}

typedef struct
{
  az_span option;
} az_storage_blobs_blob_download_options;

/**
 * @brief Creates a new blob
 *
 * @param client a storage blobs client structure
 * @param content blob content
 * @param options create options for blob. It can be NULL so nothing is added to http request
 * headers
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_storage_blobs_blob_create
 */
AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_span content, /* Buffer of content*/
    az_storage_blobs_blob_upload_options* options,
    az_http_response* response);

#include <_az_cfg_suffix.h>

#endif // _az_STORAGE_BLOBS_H
