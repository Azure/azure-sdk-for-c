// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_H
#define AZ_KEYVAULT_H

#include <az_credential.h>
#include <az_http_pipeline.h>

#include <stdlib.h>

#define AZ_KEY_VAULT_KEY_TYPE_NONE_STR ""

#include <_az_cfg_prefix.h>

/**
 * @brief Key Type represent the string used in url path.
 * Instead of asking the right string to user, client will map it from
 * one of this enum values.
 */
typedef enum {
  AZ_KEY_VAULT_KEY_TYPE_NONE = 0,
  AZ_KEY_VAULT_KEY_TYPE_KEY = 1,
  AZ_KEY_VAULT_KEY_TYPE_SECRET = 2,
  AZ_KEY_VAULT_KEY_TYPE_CERTIFICATE = 3,
} az_key_vault_key_type;

typedef struct {
  az_span const version;
} az_keyvault_keys_client_options;

typedef struct {
  // key size(size_t, typical default is 4096)
  // Expires date
  // All the REST options
  az_span option;
} az_keyvault_keys_keys_options;

typedef struct {
  az_span uri;
  az_http_pipeline pipeline;
  az_span version;
} az_keyvault_keys_client;

AZ_NODISCARD AZ_INLINE az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * const client,
    az_span const uri,
    az_credential * const credential,
    az_keyvault_keys_client_options const * const options) {
  AZ_CONTRACT_ARG_NOT_NULL(client);

  client->uri = uri;
  client->version = options->version;
  client->pipeline = (az_http_pipeline){
    .policies = {
      { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_retry, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_authentication, .data = credential },
      { .pfnc_process = az_http_pipeline_policy_logging,.data = NULL },
      { .pfnc_process = az_http_pipeline_policy_bufferresponse,.data = NULL },
      { .pfnc_process = az_http_pipeline_policy_distributedtracing,.data = NULL },
      { .pfnc_process = az_http_pipeline_policy_transport,.data = NULL },
      { .pfnc_process = NULL, .data = NULL },
    }, 
    };
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client const * const client,
    az_span const key_name,
    az_span const key_type,
    az_keyvault_keys_keys_options const * const options,
    az_mut_span const * const out);

AZ_NODISCARD az_result az_keyvault_keys_key_get(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_key_vault_key_type const key_type,
    az_mut_span const * const out);

#include <_az_cfg_suffix.h>

#endif
