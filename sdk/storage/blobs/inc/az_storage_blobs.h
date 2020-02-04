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

static az_span const AZ_STORAGE_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("2019-02-02");
typedef struct {
  az_http_policy_retry_options retry;
  struct {
    az_http_client http_client;
    _az_http_policy_apiversion_options api_version;
  } _internal;
} az_storage_blobs_blob_client_options;

AZ_NODISCARD az_storage_blobs_blob_client_options az_storage_blobs_blob_client_options_default();
typedef struct {
  struct {
    az_span uri;
    az_http_pipeline pipeline;
    az_storage_blobs_blob_client_options options;

    az_identity_access_token _token;
    az_identity_access_token_context _token_context;
  } _internal;
} az_storage_blobs_blob_client;

AZ_NODISCARD az_result az_storage_blobs_blob_client_init(
    az_storage_blobs_blob_client * client,
    az_span uri,
    void * credential,
    az_storage_blobs_blob_client_options * options);

typedef struct {
  az_span option;
} az_storage_blobs_blob_upload_options;

AZ_NODISCARD AZ_INLINE az_storage_blobs_blob_upload_options
az_storage_blobs_blob_upload_options_default() {
  return (az_storage_blobs_blob_upload_options){ .option = az_span_null() };
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
    az_storage_blobs_blob_upload_options * options,
    az_http_response * response);

#include <_az_cfg_suffix.h>

#endif
