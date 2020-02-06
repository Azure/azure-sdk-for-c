// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_KEYVAULT_H
#define _az_KEYVAULT_H

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_identity_access_token.h>
#include <az_identity_access_token_context.h>
#include <az_optional_types.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief SDKs are specific to a fixed version of the KeyVault service
 */
static az_span const AZ_KEYVAULT_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("7.0");

typedef struct {
  az_http_policy_retry_options retry;
  struct {
    _az_http_policy_apiversion_options api_version;
    _az_http_policy_telemetry_options _telemetry_options;
    az_http_client http_client;
  } _internal;
} az_keyvault_keys_client_options;

/**
 * @brief Init a client with default options
 * This is convinient method to create a client with basic settings
 * Options can be updated specifally after this for unique customization
 *
 * Use this, for instance, when only caring about setting one option by calling this method and then
 * overriding that specific option
 */
AZ_NODISCARD az_keyvault_keys_client_options
az_keyvault_keys_client_options_default(az_http_client http_client);

typedef struct {
  struct {
    // buffer to copy customer url. Then it stays immutable
    uint8_t url_buffer[AZ_HTTP_URL_MAX_SIZE];
    // this url will point to url_buffer
    az_span uri;
    az_http_pipeline pipeline;
    az_keyvault_keys_client_options options;

    az_identity_access_token _token;
    az_identity_access_token_context _token_context;
  } _internal;
} az_keyvault_keys_client;

AZ_NODISCARD az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * self,
    az_span uri,
    void * credential,
    az_keyvault_keys_client_options * options);

typedef az_span json_web_key_type;
AZ_NODISCARD AZ_INLINE json_web_key_type az_keyvault_web_key_type_EC() {
  return AZ_SPAN_FROM_STR("EC");
}
AZ_NODISCARD AZ_INLINE json_web_key_type az_keyvault_web_key_type_EC_HSM() {
  return AZ_SPAN_FROM_STR("EC-HSM");
}
AZ_NODISCARD AZ_INLINE json_web_key_type az_keyvault_web_key_type_RSA() {
  return AZ_SPAN_FROM_STR("RSA");
}
AZ_NODISCARD AZ_INLINE json_web_key_type az_keyvault_web_key_type_RSA_HSM() {
  return AZ_SPAN_FROM_STR("RSA-HSM");
}
AZ_NODISCARD AZ_INLINE json_web_key_type az_keyvault_web_key_type_OCT() {
  return AZ_SPAN_FROM_STR("oct");
}

typedef az_span az_keyvault_key_operation;
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_decrypt() {
  return AZ_SPAN_FROM_STR("decrypt");
}
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_encrypt() {
  return AZ_SPAN_FROM_STR("encrypt");
}
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_sign() {
  return AZ_SPAN_FROM_STR("sign");
}
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_unwrapKey() {
  return AZ_SPAN_FROM_STR("unwrapKey");
}
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_verify() {
  return AZ_SPAN_FROM_STR("verify");
}
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_wrapKey() {
  return AZ_SPAN_FROM_STR("wrapKey");
}
AZ_NODISCARD AZ_INLINE az_keyvault_key_operation az_keyvault_key_operation_null() {
  return az_span_null();
}

typedef struct {
  az_optional_bool enabled;
  az_keyvault_key_operation * operations;
  az_pair * tags;
  /* TODO: adding next options
  Datetime not_before;
  Datetime expires_on
  */
} az_keyvault_create_key_options;

AZ_NODISCARD az_keyvault_create_key_options az_keyvault_create_key_options_default();

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
    az_span key_name,
    json_web_key_type json_web_key_type,
    az_keyvault_create_key_options * options,
    az_http_response * response);

/**
 * @brief Gets the public part of a stored key.
 * The get key operation is applicable to all key types. If the requested key is symmetric, then no
 * key material is released in the response. This operation requires the keys/get permission.
 *
 * Get latest version by passing az_span_null as value for version
 *
 * @param client a keyvault client structure
 * @param key_name name of key to be retrieved
 * @param key_version specific key version to get. It can be null to get latest version
 * @param response a pre allocated buffer where to write http response
 * @return AZ_NODISCARD az_keyvault_keys_key_get
 */
AZ_NODISCARD az_result az_keyvault_keys_key_get(
    az_keyvault_keys_client * client,
    az_span key_name,
    az_span key_version,
    az_http_response * response);

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
    az_span key_name,
    az_http_response * response);

#include <_az_cfg_suffix.h>

#endif
