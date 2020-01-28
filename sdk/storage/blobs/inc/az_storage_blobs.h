// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_STORAGE_BLOBS_H
#define _az_STORAGE_BLOBS_H

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_identity_access_token.h>
#include <az_identity_access_token_context.h>
#include <az_result.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

static az_span const AZ_STORAGE_BLOBS_BLOB_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("2019-02-02");

static az_span const AZ_STORAGE_BLOBS_BLOB_HEADER_X_MS_BLOB_TYPE
    = AZ_SPAN_LITERAL_FROM_STR("x-ms-blob-type");
static az_span const AZ_STORAGE_BLOBS_BLOB_TYPE_APPENDBLOB = AZ_SPAN_LITERAL_FROM_STR("AppendBlob");
static az_span const AZ_STORAGE_BLOBS_BLOB_TYPE_BLOCKBLOB = AZ_SPAN_LITERAL_FROM_STR("BlockBlob");
static az_span const AZ_STORAGE_BLOBS_BLOB_TYPE_PAGEBLOB = AZ_SPAN_LITERAL_FROM_STR("PageBlob");

typedef struct {
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
  az_identity_access_token _token;
  az_identity_access_token_context _token_context;
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

  *client = (az_storage_blobs_blob_client){
    .uri = uri,
    .pipeline = { 0 },
    .client_options = options == NULL ? AZ_STORAGE_BLOBS_BLOB_CLIENT_DEFAULT_OPTIONS : *options,
    ._token = { 0 },
    ._token_context = { 0 },
  };

  AZ_RETURN_IF_FAILED(az_identity_access_token_init(&(client->_token)));
  AZ_RETURN_IF_FAILED(az_identity_access_token_context_init(
      &(client->_token_context),
      credential,
      &(client->_token),
      AZ_SPAN_FROM_STR("https://storage.azure.com/.default")));

  client->pipeline = (az_http_pipeline){
    .policies = {
      { .process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
      { .process = az_http_pipeline_policy_retry, .data = &client->client_options.retry },
      { .process = az_http_pipeline_policy_authentication, .data = &(client->_token_context) },
      { .process = az_http_pipeline_policy_logging, .data = NULL },
      { .process = az_http_pipeline_policy_bufferresponse, .data = NULL },
      { .process = az_http_pipeline_policy_distributedtracing, .data = NULL },
      { .process = az_http_pipeline_policy_transport, .data = NULL },
      { .process = NULL, .data = NULL },
    }, 
    };

  // Currently only BlockBlobs are supported
  client->blob_type = AZ_STORAGE_BLOBS_BLOB_TYPE_BLOCKBLOB;

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
    az_span const content, /* Buffer of content*/
    az_storage_blobs_blob_upload_options * const options,
    az_http_response * const response);

#include <_az_cfg_suffix.h>

#endif
