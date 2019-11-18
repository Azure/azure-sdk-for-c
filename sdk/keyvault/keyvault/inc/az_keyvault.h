// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_H
#define AZ_KEYVAULT_H

#include <az_contract.h>
#include <az_credential.h>
#include <az_http_pipeline.h>
#include <az_http_request_builder.h>
#include <az_result.h>
#include <az_span.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg_prefix.h>

extern az_span const AZ_KEY_VAULT_NONE_TYPE;
extern az_span const AZ_KEY_VAULT_KEY_TYPE;
extern az_span const AZ_KEY_VAULT_SECRET_TYPE;
extern az_span const AZ_KEY_VAULT_CERTIFICATE_TYPE;
extern az_span const AZ_KEY_VAULT_URL_PATH_SEPARATOR;
typedef enum {
  AZ_KEY_VAULT_NONE = 0,
  AZ_KEY_VAULT_KEY = 1,
  AZ_KEY_VAULT_SECRET = 2,
  AZ_KEY_VAULT_CERTIFICATE = 3,
} az_key_vault_key_type;

typedef struct {
  az_span version;
} az_keyvault_keys_client_options;

typedef struct {
  az_http_policy pipeline[8];
} az_pipeline_policies;

typedef struct {
  az_span uri;
  az_pipeline_policies pipeline_policies;
  az_span version;
} az_keyvault_keys_client;

typedef struct {
  // key size(size_t, typical default is 4096)
  // Expires date
  // All the REST options
  az_span option;
} az_keyvault_keys_keys_options;

AZ_NODISCARD AZ_INLINE az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * client,
    az_span uri,
    az_credential * auth,
    az_keyvault_keys_client_options * options) {
  client->uri = uri;
  client->version = options->version;
  client->pipeline_policies = (az_pipeline_policies){
    .pipeline = {
      { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_retry, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_authentication, .data = auth },
      { .pfnc_process = az_http_pipeline_policy_logging,.data = NULL },
      { .pfnc_process = az_http_pipeline_policy_bufferresponse,.data = NULL },
      { .pfnc_process = az_http_pipeline_policy_distributedtracing,.data = NULL },
      { .pfnc_process = az_http_pipeline_policy_transport,.data = NULL },
      { .pfnc_process = NULL, .data = NULL },
    }, 
    };
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_createKey(
    az_keyvault_keys_client * client,
    az_span keyname,
    az_span keytype,
    az_keyvault_keys_keys_options * options,
    const az_mut_span * const out);

AZ_NODISCARD az_result az_keyvault_keys_getKey(
    az_keyvault_keys_client const * const client,
    az_span const keyname,
    az_key_vault_key_type const keytype,
    const az_mut_span * const out);

AZ_NODISCARD AZ_INLINE az_span az_keyvault_get_key_type_span(az_key_vault_key_type const keytype) {
  switch (keytype) {
    case AZ_KEY_VAULT_KEY:
      return AZ_KEY_VAULT_KEY_TYPE;
      break;

    case AZ_KEY_VAULT_SECRET:
      return AZ_KEY_VAULT_SECRET_TYPE;
      break;

    case AZ_KEY_VAULT_CERTIFICATE:
      return AZ_KEY_VAULT_CERTIFICATE_TYPE;
      break;

    default:
      return AZ_KEY_VAULT_NONE_TYPE;
  }
}

#include <_az_cfg_suffix.h>

#endif
