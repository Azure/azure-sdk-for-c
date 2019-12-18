// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STORAGE_BLOBS_H
#define AZ_STORAGE_BLOBS_H

#include <az_contract.h>
#include <az_http_pipeline.h>
#include <az_http_response.h>
#include <az_result.h>
#include <az_span.h>
#include <az_str.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_blob_header_blob_type() {
  return AZ_STR("x-ms-blob-type");
}
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_blob_header_content_length() {
  return AZ_STR("Content_Length");
}

AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_blob_type_append_blob() {
  return AZ_STR("AppendBlob");
} /* UnSupported */
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_blob_type_block_blob() {
  return AZ_STR("BlockBlob");
}
AZ_NODISCARD AZ_INLINE az_span az_storage_blobs_blob_type_page_blob() {
  return AZ_STR("PageBlob");
} /* UnSupported */

typedef struct {
  az_span service_version;
  az_http_policy_retry_options retry;
} az_storage_blobs_blob_client_options;

typedef struct {
  az_span option;
} az_storage_blobs_blob_upload_options;

typedef struct {
  az_span option;
} az_storage_blobs_blob_download_options;

typedef struct {
  az_span uri;
  az_span blob_type;
  az_http_pipeline pipeline;
  az_storage_blobs_blob_client_options client_options;
} az_storage_blobs_blob_client;

extern az_storage_blobs_blob_client_options const AZ_STORAGE_BLOBS_BLOB_CLIENT_DEFAULT_OPTIONS;

/**
 * @brief Init a client with default options
 * This is convinient method to create a client with basic settings
 * Options can be updated specifally after this for unique customization
 *
 * Use this, for instance, when only caring about setting one option by calling this method and then
 * overriding that specific option
 */
AZ_NODISCARD AZ_INLINE az_result
az_storage_blobs_blob_client_options_init(az_storage_blobs_blob_client_options * const options) {
  AZ_CONTRACT_ARG_NOT_NULL(options);
  *options = AZ_STORAGE_BLOBS_BLOB_CLIENT_DEFAULT_OPTIONS;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_storage_blobs_blob_client_init(
    az_storage_blobs_blob_client * const client,
    az_span const uri,
    void * const credential,
    az_storage_blobs_blob_client_options const * const options) {
  AZ_CONTRACT_ARG_NOT_NULL(client);

  client->uri = uri;
  // use default options if options is null. Or use customer provided one
  if (options == NULL) {
    client->client_options = AZ_STORAGE_BLOBS_BLOB_CLIENT_DEFAULT_OPTIONS;
  } else {
    client->client_options = *options;
  }

  client->pipeline = (az_http_pipeline){
    .policies = {
      { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_retry, .data = &client->client_options.retry },
      { .pfnc_process = az_http_pipeline_policy_authentication, .data = credential },
      { .pfnc_process = az_http_pipeline_policy_logging, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_transport, .data = NULL },
      { .pfnc_process = NULL, .data = NULL },
    }, 
    };

  // Currently only BlockBlobs are supported
  client->blob_type = az_storage_blobs_blob_type_BlockBlob();

  return AZ_OK;
}

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
    az_storage_blobs_blob_client * client,
    az_span content, /* Buffer of content*/
    az_storage_blobs_blob_upload_options * const options,
    az_http_response * const response);

// TODO: IOT customers want to be able to resume

/**
 * @brief Gets a blob
 *
 * @param client a storage blobs client structure
 * @param blob_name name of blob to be retrieved
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_storage_blobs_blob_get
 */
AZ_NODISCARD az_result az_storage_blobs_blob_download(
    az_storage_blobs_blob_client * client,
    az_span_builder content,
    az_storage_blobs_blob_download_options options,
    az_http_response * const response);

// Http header supported
//   Range header as option for get
//

/**
 * @brief Deletes a blob
 *
 * @param client a storage blobs client structure
 * @param blob_name name of the blob to be deleted
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_storage_blobs_blob_delete
 */
AZ_NODISCARD az_result az_storage_blobs_blob_delete(
    az_storage_blobs_blob_client * client,
    az_http_response * const response);

#include <_az_cfg_suffix.h>

#endif
