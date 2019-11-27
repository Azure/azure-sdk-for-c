// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_H
#define AZ_KEYVAULT_H

#include <az_credential.h>
#include <az_http_pipeline.h>

#include <stdlib.h>

#define AZ_KEYVAULT_KEY_TYPE_NONE_STR ""

#include <_az_cfg_prefix.h>

/**
 * @brief Key Type represent the string used in url path.
 * Instead of asking the right string to user, client will map it from
 * one of this enum values.
 */
typedef enum {
  AZ_KEYVAULT_KEY_TYPE_NONE = 0,
  AZ_KEYVAULT_KEY_TYPE_KEY = 1,
  AZ_KEYVAULT_KEY_TYPE_SECRET = 2,
  AZ_KEYVAULT_KEY_TYPE_CERTIFICATE = 3,
} az_keyvault_key_type;

typedef enum {
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_NONE = 0,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_EC = 1,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_EC_HSM = 2,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA = 3,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA_HSM = 4,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_OCT = 5,
} az_keyvault_json_web_key_type;

typedef struct {
  az_span service_version;
  az_keyvault_keys_client_options_retry retry;
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
    az_keyvault_keys_client * const client,
    az_span const uri,
    void * const credential,
    az_keyvault_keys_client_options const * const options) {
  AZ_CONTRACT_ARG_NOT_NULL(client);

  client->uri = uri;
  // use default options if options is null. Or use customer provided one
  if (options == NULL) {
    client->retry_options = AZ_KEYVAULT_CLIENT_DEFAULT_OPTIONS;
  } else {
    client->retry_options = *options;
  }

  client->pipeline = (az_http_pipeline){
    .policies = {
      { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
      { .pfnc_process = az_http_pipeline_policy_retry, .data = &client->retry_options.retry },
      { .pfnc_process = az_http_pipeline_policy_authentication, .data = credential },
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
 * @param client
 * @param key_name
 * @param json_web_key_type
 * @param options
 * @param response
 * @return AZ_NODISCARD az_keyvault_keys_key_create
 */
AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_keyvault_json_web_key_type const json_web_key_type,
    az_keyvault_keys_keys_options const * const options,
    az_http_response * const response);

/**
 * @brief Gets the public part of a stored key.
 * The get key operation is applicable to all key types. If the requested key is symmetric, then no
 * key material is released in the response. This operation requires the keys/get permission.
 *
 * @param client
 * @param key_name
 * @param key_type
 * @param response
 * @return AZ_NODISCARD az_keyvault_keys_key_get
 */
AZ_NODISCARD az_result az_keyvault_keys_key_get(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_keyvault_key_type const key_type,
    az_http_response * const response);

/**
 * @brief Deletes a key of any type from storage in Azure Key Vault.
 * The delete key operation cannot be used to remove individual versions of a key. This operation
 * removes the cryptographic material associated with the key, which means the key is not usable for
 * Sign/Verify, Wrap/Unwrap or Encrypt/Decrypt operations. This operation requires the keys/delete
 * permission.
 *
 * @param client
 * @param key_name
 * @param response
 * @return AZ_NODISCARD az_keyvault_keys_key_delete
 */
AZ_NODISCARD az_result az_keyvault_keys_key_delete(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_http_response const * const response);

#include <_az_cfg_suffix.h>

#endif
