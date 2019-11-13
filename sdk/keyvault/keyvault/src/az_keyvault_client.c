// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <_az_cfg.h>

// TODO: Remove clientID from URL and move this to options and let Pipeline to build this
static az_span const KEY_VAULT_URL = AZ_CONST_STR("https://testingforc99.vault.azure.net");
static uint16_t const MAX_URL_SIZE = 80;

az_span const AZ_KEY_VAULT_NONE_TYPE = AZ_CONST_STR("");
az_span const AZ_KEY_VAULT_KEY_TYPE = AZ_CONST_STR("keys");
az_span const AZ_KEY_VAULT_SECRET_TYPE = AZ_CONST_STR("secrets");
az_span const AZ_KEY_VAULT_CERTIFICATE_TYPE = AZ_CONST_STR("certificates");
az_span const AZ_KEY_VAULT_URL_PATH_SEPARATOR = AZ_CONST_STR("/");

// Note: Options can be NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * client,
    az_span uri,
    /*Azure Credentials */
    az_keyvault_keys_client_options * options) {
  (void)client;
  (void)uri;
  (void)options;

  return AZ_OK;
}

// TODO #define AZ_KEYVAULT_KEYS_KEYTYPE_XXXX
// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_createKey(
    az_keyvault_keys_client * client,
    az_span keyname,
    az_span keytype,
    az_keyvault_keys_keys_options * options,
    const az_mut_span * const out) {
  (void)client;
  (void)keyname;
  (void)keytype;
  (void)options;
  (void)out;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_keyvault_build_url(az_span const key_type, az_span const key_name, az_mut_span const out) {
  az_span_builder s_builder = az_span_builder_create(out);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, KEY_VAULT_URL));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, AZ_KEY_VAULT_URL_PATH_SEPARATOR));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, key_type));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, AZ_KEY_VAULT_URL_PATH_SEPARATOR));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, key_name));
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_getKey(
    az_keyvault_keys_client const * const client,
    az_span const keyname,
    az_key_vault_key_type const keytype,
    const az_mut_span * const out) {
  (void)client;
  (void)keyname;
  (void)keytype;

  // create request buffer TODO: define size for a getKey Request
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  /* ******** build url for request  ******
   * add keytype and keyname to url request
   */
  az_span const az_key_type_span = az_keyvault_get_key_type_span(keytype);
  // stack a new buffer to hold url with key name and type
  uint8_t url_buffer
      [KEY_VAULT_URL.size + (AZ_KEY_VAULT_URL_PATH_SEPARATOR.size * 2) + az_key_type_span.size
       + keyname.size];
  az_mut_span const url_buffer_span = AZ_SPAN_FROM_ARRAY(url_buffer);
  AZ_RETURN_IF_FAILED(az_keyvault_build_url(az_key_type_span, keyname, url_buffer_span));

  printf("r: %s", url_buffer_span.begin);

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      az_mut_span_to_span(url_buffer_span)));

  // start pipeline
  return az_http_pipeline_process(&hrb, out);
}
