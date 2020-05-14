// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_storage_blobs.h
 *
 * @brief Definition for the Azure Storage Blobs client.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_STORAGE_BLOBS_H
#define _az_STORAGE_BLOBS_H

#include <az_config.h>
#include <az_context.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Client is fixed to a specific version of the Azure Storage Blobs service
 */
static az_span const AZ_STORAGE_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("2019-02-02");

typedef struct
{
  az_http_policy_retry_options retry;
  struct
  {
    _az_http_policy_apiversion_options api_version;
    _az_http_policy_telemetry_options _telemetry_options;
  } _internal;
} az_storage_blobs_blob_client_options;

typedef struct
{
  struct
  {
    // buffer to copy customer url. Then it stays immutable
    uint8_t endpoint_buffer[AZ_HTTP_REQUEST_URL_BUF_SIZE];
    // this url will point to endpoint_buffer
    az_span endpoint;
    _az_http_pipeline pipeline;
    az_storage_blobs_blob_client_options options;
    _az_credential* credential;
  } _internal;
} az_storage_blobs_blob_client;

/**
 * @brief Initialize a client with default options.
 *
 * @param client The blob client instance to initialize.
 * @param endpoint A url to a blob storage account.
 * @param credential The object used for authentication.
 *         #AZ_CREDENTIAL_ANONYMOUS should be used for SAS.
 * @param options  A reference to an #az_storage_blobs_blob_client_options structure which defines
 * custom behavior of the client.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result az_storage_blobs_blob_client_init(
    az_storage_blobs_blob_client* client,
    az_span endpoint,
    void* credential,
    az_storage_blobs_blob_client_options* options);

typedef struct
{
  struct
  {
    az_span option;
  } _internal;
} az_storage_blobs_blob_upload_options;

/**
 * @brief Gets the default blob storage options.
 *
 * @details Call this to obtain an initialized #az_storage_blobs_blob_client_options structure that
 * can be modified and passed to #az_storage_blobs_blob_client_init().
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this method
 * and then overriding that specific option.
 */
AZ_NODISCARD az_storage_blobs_blob_client_options az_storage_blobs_blob_client_options_default();

/**
 * @brief Gets the default blob upload options
 *
 * @details Call this to obtain an initialized #az_storage_blobs_blob_upload_options structure
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this method
 * and then overriding that specific option.
 *
 */
AZ_NODISCARD AZ_INLINE az_storage_blobs_blob_upload_options
az_storage_blobs_blob_upload_options_default()
{
  return (az_storage_blobs_blob_upload_options){ ._internal = { .option = AZ_SPAN_NULL } };
}

/**
 * @brief Uploads the contents to blob storage.
 *
 * @param client A storage blobs client structure.
 * @param context Supports cancelling long running operations.
 * @param content The blob content to upload.
 * @param options __[nullable]__ A reference to an #az_storage_blobs_blob_upload_options
 * structure which defines custom behavior for uploading the blob. If `NULL` is passed, the client
 * will use the default options (i.e. #az_storage_blobs_blob_upload_options_default()).
 * @param response A pre-allocated buffer where to write HTTP response into.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_span content, /* Buffer of content*/
    az_storage_blobs_blob_upload_options* options,
    az_http_response* response);

#include <_az_cfg_suffix.h>

#endif // _az_STORAGE_BLOBS_H
