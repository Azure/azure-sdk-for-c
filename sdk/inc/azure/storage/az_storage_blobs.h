// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition for the Azure Blob Storage blob client.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_STORAGE_BLOBS_H
#define _az_STORAGE_BLOBS_H

#include <azure/core/az_config.h>
#include <azure/core/az_context.h>
#include <azure/core/az_credentials.h>
#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_http_internal.h>

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Allows customization of the blob client.
 */
typedef struct
{
  struct
  {
    /// Telemetry options.
    _az_http_policy_telemetry_options telemetry_options;

    /// Service API version.
    _az_http_policy_apiversion_options api_version;
  } _internal;

  /// Optional values used to override the default retry policy options.
  az_http_policy_retry_options retry_options;
} az_storage_blobs_blob_client_options;

/**
 * @brief Gets the default blob storage options.
 *
 * @details Call this to obtain an initialized #az_storage_blobs_blob_client_options structure that
 * can be modified and passed to #az_storage_blobs_blob_client_init().
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this
 * function and then overriding that specific option.
 */
AZ_NODISCARD az_storage_blobs_blob_client_options az_storage_blobs_blob_client_options_default();

/**
 * @brief Azure Blob Storage Blob Client.
 */
typedef struct
{
  struct
  {
    _az_credential* credential;
    _az_http_pipeline pipeline;
    az_span blob_url;
    az_span host;
    az_storage_blobs_blob_client_options options;
    uint8_t blob_url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE];
  } _internal;
} az_storage_blobs_blob_client;

/**
 * @brief Initialize a client with default options.
 *
 * @param[out] out_client The blob client instance to initialize.
 * @param[in] blob_url A blob URL. Must be a vaild URL, cannot be empty.
 * @param credential The object used for authentication. #AZ_CREDENTIAL_ANONYMOUS should be
 * used for SAS.
 * @param[in] options __[nullable]__ A reference to an #az_storage_blobs_blob_client_options
 * structure which defines custom behavior of the client. If `NULL` is passed, the client
 * will use the default options (i.e. #az_storage_blobs_blob_client_options_default()).
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result az_storage_blobs_blob_client_init(
    az_storage_blobs_blob_client* out_client,
    az_span blob_url,
    void* credential,
    az_storage_blobs_blob_client_options const* options);

/**
 * @brief Allows customization of the upload operation.
 */
typedef struct
{
  struct
  {
    bool unused;
  } _internal;
} az_storage_blobs_blob_upload_options;

/**
 * @brief Allows customization of the download operation.
 */
typedef struct
{
  struct
  {
    bool unused;
  } _internal;
} az_storage_blobs_blob_download_options;

/**
 * @brief Gets the default blob upload options.
 *
 * @details Call this to obtain an initialized #az_storage_blobs_blob_upload_options structure.
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this
 * function and then overriding that specific option.
 */
AZ_NODISCARD AZ_INLINE az_storage_blobs_blob_upload_options
az_storage_blobs_blob_upload_options_default()
{
  return (az_storage_blobs_blob_upload_options){ 0 };
}

/**
 * @brief Gets the default blob download options.
 *
 * @details Call this to obtain an initialized #az_storage_blobs_blob_download_options structure.
 *
 * @remark Use this, for instance, when only caring about setting one option by calling this
 * function and then overriding that specific option.
 */
AZ_NODISCARD AZ_INLINE az_storage_blobs_blob_download_options
az_storage_blobs_blob_download_options_default()
{
  return (az_storage_blobs_blob_download_options){ 0 };
}

/**
 * @brief Uploads a span contents to blob storage.
 *
 * @param[in] client An #az_storage_blobs_blob_client structure.
 * @param[in] context __[nullable]__ A context to control the request lifetime. If `NULL` is passed,
 * #az_context_application is used.
 * @param[in] content The blob content to upload.
 * @param[in] options __[nullable]__ A reference to an #az_storage_blobs_blob_upload_options
 * structure which defines custom behavior for uploading the blob. If `NULL` is passed, the client
 * will use the default options (i.e. #az_storage_blobs_blob_upload_options_default()).
 * @param[in,out] ref_response An initialized #az_http_response where to write HTTP response into.
 * See https://docs.microsoft.com/rest/api/storageservices/put-blob#response
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_span content,
    az_storage_blobs_blob_upload_options const* options,
    az_http_response* ref_response);

/**
 * @brief Downloads the blob.
 *
 * @param[in] client An #az_storage_blobs_blob_client structure.
 * @param[in] context __[nullable]__ A context to control the request lifetime. If `NULL` is passed,
 * #az_context_application is used.
 * @param[in] options __[nullable]__ A reference to an #az_storage_blobs_blob_download_options
 * structure which defines custom behavior for downloading the blob. If `NULL` is passed, the client
 * will use the default options (i.e. #az_storage_blobs_blob_download_options_default()).
 * @param[in,out] ref_response An initialized #az_http_response where to write HTTP response into.
 * See https://docs.microsoft.com/rest/api/storageservices/get-blob#response.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result az_storage_blobs_blob_download(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_storage_blobs_blob_download_options const* options,
    az_http_response* ref_response);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_STORAGE_BLOBS_H
