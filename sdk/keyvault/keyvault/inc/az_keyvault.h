// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_H
#define AZ_KEYVAULT_H

#include <az_access_token.h>
#include <az_access_token_context.h>
#include <az_contract.h>
#include <az_http_pipeline.h>
#include <az_http_response.h>
#include <az_keyvault_create_key_options.h>
#include <az_result.h>
#include <az_span.h>
#include <az_str.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span service_version;
  az_http_policy_retry_options retry;
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
  az_keyvault_keys_client_options retry_options;
  az_access_token _token;
  az_access_token_context _token_context;
} az_keyvault_keys_client;

extern az_keyvault_keys_client_options const AZ_KEYVAULT_CLIENT_DEFAULT_OPTIONS;

/**
 * @brief Init a client with default options
 * This is convinient method to create a client with basic settings
 * Options can be updated specifally after this for unique customization
 *
 * Use this, for instance, when only caring about setting one option by calling this method and then
 * overriding that specific option
 */
AZ_NODISCARD AZ_INLINE az_result
az_keyvault_keys_client_options_init(az_keyvault_keys_client_options * const options) {
  AZ_CONTRACT_ARG_NOT_NULL(options);
  *options = AZ_KEYVAULT_CLIENT_DEFAULT_OPTIONS;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * const self,
    az_span const uri,
    void * const credential,
    az_keyvault_keys_client_options const * const options) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  *self = (az_keyvault_keys_client){
    .uri = uri,
    .pipeline = { 0 },
    .retry_options = options == NULL ? AZ_KEYVAULT_CLIENT_DEFAULT_OPTIONS : *options,
    ._token = { 0 },
    ._token_context = { 0 },
  };

  AZ_RETURN_IF_FAILED(az_access_token_init(&(self->_token)));
  AZ_RETURN_IF_FAILED(az_access_token_context_init(
      &(self->_token_context),
      credential,
      &(self->_token),
      AZ_STR("https://vault.azure.net/.default")));

  self->pipeline = (az_http_pipeline){
    .policies = {
      { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_retry, .data = &(self->retry_options.retry) },
      { .pfnc_process = az_http_pipeline_policy_authentication, .data = &(self->_token_context) },
      { .pfnc_process = az_http_pipeline_policy_logging, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_transport, .data = NULL },
      { .pfnc_process = NULL, .data = NULL },
    }, 
    };

  return AZ_OK;
}

/**
 * @brief Creates a new key, stores it, then returns key parameters and attributes to the client.
 * The create key operation can be used to create any key type in Azure Key Vault. If the named key
 * already exists, Azure Key Vault creates a new version of the key. It requires the keys/create
 * permission.
 *
 * @param client a keyvault client structure
 * @param key_name name for key to be created
 * @param json_web_key_type type of key to create
 * @param options create options for key. It can be NULL so nothing is added to http request body
 * and server will use defaults to create key
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_keyvault_keys_key_create
 */
AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_span const json_web_key_type,
    az_keyvault_create_key_options * const options,
    az_http_response * const response);

/**
 * @brief Gets the public part of a stored key.
 * The get key operation is applicable to all key types. If the requested key is symmetric, then no
 * key material is released in the response. This operation requires the keys/get permission.
 *
 * Get latest version by passing az_span_null as value for version
 *
 * @param client a keyvault client structure
 * @param key_name name of key to be retrieved
 * @param version specific key version to get. It can be null to get latest version
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_keyvault_keys_key_get
 */
AZ_NODISCARD az_result az_keyvault_keys_key_get(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_span const version,
    az_http_response * const response);

/**
 * @brief Deletes a key of any type from storage in Azure Key Vault.
 * The delete key operation cannot be used to remove individual versions of a key. This operation
 * removes the cryptographic material associated with the key, which means the key is not usable for
 * Sign/Verify, Wrap/Unwrap or Encrypt/Decrypt operations. This operation requires the keys/delete
 * permission.
 *
 * @param client a keyvault client structure
 * @param key_name name of the key to be deleted
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_keyvault_keys_key_delete
 */
AZ_NODISCARD az_result az_keyvault_keys_key_delete(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_http_response * const response);

AZ_NODISCARD AZ_INLINE az_span az_keyvault_web_key_type_EC() { return AZ_STR("EC"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_web_key_type_EC_HSM() { return AZ_STR("EC-HSM"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_web_key_type_RSA() { return AZ_STR("RSA"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_web_key_type_RSA_HSM() { return AZ_STR("RSA-HSM"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_web_key_type_OCT() { return AZ_STR("oct"); }

#include <_az_cfg_suffix.h>

#endif
