// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <_az_cfg.h>

/**
 * @brief Maximum allowed URL size:
 * url is expected as : [https://]{account_id}[.vault.azure.net]{path}{query}
 * URL token                       max Len            Total
 * [https://]                       = 8                 8
 * {account_id}                     = 52               60
 * [.vault.azure.net]               = 16               76
 * {path}                           = 54               130
 * {query}                          = 70               ** 200 **
 */
enum { MAX_URL_SIZE = 200 };

static az_span const AZ_KEY_VAULT_KEY_TYPE_KEY_STR = AZ_CONST_STR("keys");
static az_span const AZ_KEY_VAULT_KEY_TYPE_SECRET_STR = AZ_CONST_STR("secrets");
static az_span const AZ_KEY_VAULT_KEY_TYPE_CERTIFICATE_STR = AZ_CONST_STR("certificates");

az_keyvault_keys_client_options const AZ_KEYVAULT_CLIENT_DEFAULT_OPTIONS
    = { .service_version = AZ_CONST_STR("7.0"),
        .retry = {
            .max_retry = 3,
            .delay_in_ms = 30,
        } };

AZ_NODISCARD AZ_INLINE az_span az_keyvault_get_key_type_span(az_key_vault_key_type const key_type) {
  switch (key_type) {
    case AZ_KEY_VAULT_KEY_TYPE_KEY: {
      return AZ_KEY_VAULT_KEY_TYPE_KEY_STR;
    }

    case AZ_KEY_VAULT_KEY_TYPE_SECRET: {
      return AZ_KEY_VAULT_KEY_TYPE_SECRET_STR;
    }

    case AZ_KEY_VAULT_KEY_TYPE_CERTIFICATE: {
      return AZ_KEY_VAULT_KEY_TYPE_CERTIFICATE_STR;
    }

    default: { return az_str_to_span(AZ_KEY_VAULT_KEY_TYPE_NONE_STR); }
  }
}

// TODO #define AZ_KEYVAULT_KEYS_KEYTYPE_XXXX
// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client const * const client,
    az_span const key_name,
    az_span const key_type,
    az_keyvault_keys_keys_options const * const options,
    az_mut_span const * const out) {
  (void)client;
  (void)key_name;
  (void)key_type;
  (void)options;
  (void)out;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

static AZ_NODISCARD az_result az_keyvault_build_url(
    az_span const uri,
    az_span const key_type,
    az_span const key_name,
    az_mut_span const out) {
  az_span_builder s_builder = az_span_builder_create(out);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, uri));
  AZ_RETURN_IF_FAILED(az_span_builder_append_byte(&s_builder, '/'));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, key_type));
  AZ_RETURN_IF_FAILED(az_span_builder_append_byte(&s_builder, '/'));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, key_name));
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_key_get(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_key_vault_key_type const key_type,
    az_mut_span const * const out) {
  // create request buffer TODO: define size for a getKey Request
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  /* ******** build url for request  ******
   * add key_type, key_name and version to url request
   */
  az_span const az_key_type_span = az_keyvault_get_key_type_span(key_type);

  // make sure we can handle url within the MAX_URL_SIZE
  size_t const url_size_with_path = client->uri.size + 2 /* 2 path separators '\'*/
      + az_key_type_span.size + key_name.size;
  AZ_CONTRACT(url_size_with_path <= MAX_URL_SIZE, AZ_ERROR_BUFFER_OVERFLOW);

  // Put a new buffer on the stack to hold a url with the key name and type
  uint8_t url_buffer[MAX_URL_SIZE];
  az_mut_span const url_buffer_span
      = (az_mut_span){ .begin = url_buffer, .size = url_size_with_path };
  AZ_RETURN_IF_FAILED(
      az_keyvault_build_url(client->uri, az_key_type_span, key_name, url_buffer_span));

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      az_mut_span_to_span(url_buffer_span)));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->options.service_version));

  // start pipeline
  return az_http_pipeline_process(&hrb, out, &client->pipeline);
}
