// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>
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
enum { MAX_BODY_SIZE = 1024 };

static az_span const AZ_KEY_VAULT_KEY_TYPE_KEY_STR = AZ_CONST_STR("keys");
static az_span const AZ_KEY_VAULT_KEY_TYPE_SECRET_STR = AZ_CONST_STR("secrets");
static az_span const AZ_KEY_VAULT_KEY_TYPE_CERTIFICATE_STR = AZ_CONST_STR("certificates");

static az_span const AZ_KEY_VAULT_WEB_KEY_TYPE_EC_STR = AZ_CONST_STR("EC");
static az_span const AZ_KEY_VAULT_WEB_KEY_TYPE_EC_HSM_STR = AZ_CONST_STR("EC-HSM");
static az_span const AZ_KEY_VAULT_WEB_KEY_TYPE_RSA_STR = AZ_CONST_STR("RSA");
static az_span const AZ_KEY_VAULT_WEB_KEY_TYPE_RSA_HSM_STR = AZ_CONST_STR("RSA-HSM");
static az_span const AZ_KEY_VAULT_WEB_KEY_TYPE_OCT_STR = AZ_CONST_STR("oct");

static az_span const AZ_KEY_VAULT_CREATE_KEY_URL_KEYS = AZ_CONST_STR("keys");
static az_span const AZ_KEY_VAULT_CREATE_KEY_URL_CREATE = AZ_CONST_STR("create");

static az_span const AZ_HTTP_REQUEST_BUILDER_HEADER_CONTENT_TYPE_LABEL
    = AZ_CONST_STR("Content-Type");
static az_span const AZ_HTTP_REQUEST_BUILDER_HEADER_CONTENT_TYPE_JSON
    = AZ_CONST_STR("application/json");

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

AZ_NODISCARD AZ_INLINE az_span
az_keyvault_get_json_web_key_type_span(az_keyvault_json_web_key_type const key_type) {
  switch (key_type) {
    case AZ_KEY_VAULT_JSON_WEB_KEY_TYPE_EC: {
      return AZ_KEY_VAULT_WEB_KEY_TYPE_EC_STR;
    }
    case AZ_KEY_VAULT_JSON_WEB_KEY_TYPE_EC_HSM: {
      return AZ_KEY_VAULT_WEB_KEY_TYPE_EC_HSM_STR;
    }
    case AZ_KEY_VAULT_JSON_WEB_KEY_TYPE_RSA: {
      return AZ_KEY_VAULT_WEB_KEY_TYPE_RSA_STR;
    }
    case AZ_KEY_VAULT_JSON_WEB_KEY_TYPE_RSA_HSM: {
      return AZ_KEY_VAULT_WEB_KEY_TYPE_RSA_HSM_STR;
    }
    case AZ_KEY_VAULT_JSON_WEB_KEY_TYPE_OCT: {
      return AZ_KEY_VAULT_WEB_KEY_TYPE_OCT_STR;
    }
    default: { return az_str_to_span(AZ_KEY_VAULT_KEY_TYPE_NONE_STR); }
  }
}

AZ_INLINE AZ_NODISCARD az_result az_keyvault_build_url_for_create_key(
    az_span const uri,
    az_span const key_name,
    az_span_builder * const s_builder) {
  AZ_RETURN_IF_FAILED(az_span_builder_append(s_builder, uri));
  AZ_RETURN_IF_FAILED(az_span_builder_append_byte(s_builder, '/'));
  AZ_RETURN_IF_FAILED(az_span_builder_append(s_builder, AZ_KEY_VAULT_CREATE_KEY_URL_KEYS));
  AZ_RETURN_IF_FAILED(az_span_builder_append_byte(s_builder, '/'));
  AZ_RETURN_IF_FAILED(az_span_builder_append(s_builder, key_name));
  AZ_RETURN_IF_FAILED(az_span_builder_append_byte(s_builder, '/'));
  AZ_RETURN_IF_FAILED(az_span_builder_append(s_builder, AZ_KEY_VAULT_CREATE_KEY_URL_CREATE));

  return AZ_OK;
}

/**
 * @brief Action that uses json builder to construct http request body used by create key
 *
 * @param kty required value to create a new key
 * @param write
 * @return AZ_NODISCARD build_request_json_body
 */
AZ_NODISCARD az_result build_request_json_body(az_span const kty, az_span_action const write) {
  az_json_builder builder = { 0 };

  AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, write));

  AZ_RETURN_IF_FAILED(az_json_builder_write(&builder, az_json_value_create_object()));
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder, AZ_STR("kty"), az_json_value_create_string(kty)));

  AZ_RETURN_IF_FAILED(az_json_builder_write_object_close(&builder));

  return AZ_OK;
}

// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_keyvault_json_web_key_type const json_web_key_type,
    az_keyvault_keys_keys_options const * const options,
    az_http_response * const response) {
  (void)options;

  // Request buffer
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  /* ******** build url for request  ******/

  // Get Json web key from type that will be used as kty value for creating key
  az_span const az_json_web_key_type_span
      = az_keyvault_get_json_web_key_type_span(json_web_key_type);

  // Allocate buffer in stack to create URL like:
  // {vaultBaseUrl}/keys/{key_name}/create
  uint8_t url_buffer[MAX_URL_SIZE];
  az_mut_span const url_buffer_span_temp = AZ_SPAN_FROM_ARRAY(url_buffer);
  az_span_builder s_builder = az_span_builder_create(url_buffer_span_temp);
  AZ_RETURN_IF_FAILED(az_keyvault_build_url_for_create_key(client->uri, key_name, &s_builder));
  // take an span from the created URL since url might be shorter than MAX_URL_SIZE
  az_span const url_buffer_span = az_span_builder_result(&s_builder);

  // Allocate buffer in stack to hold body request
  uint8_t body_buffer[MAX_BODY_SIZE];
  az_span_builder json_builder
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(body_buffer));
  AZ_RETURN_IF_FAILED(build_request_json_body(
      az_json_web_key_type_span, az_span_builder_append_action(&json_builder)));
  az_span const created_body = az_span_builder_result(&json_builder);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_POST,
      url_buffer_span,
      created_body));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->retry_options.service_version));

  // Adding header content-type json
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb,
      AZ_HTTP_REQUEST_BUILDER_HEADER_CONTENT_TYPE_LABEL,
      AZ_HTTP_REQUEST_BUILDER_HEADER_CONTENT_TYPE_JSON));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

AZ_INLINE AZ_NODISCARD az_result az_keyvault_build_url_for_get_key(
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

/**
 * @brief Currently returning last key version. Need to update to get version key
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
    az_key_vault_key_type const key_type,
    az_http_response * const response) {
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
      az_keyvault_build_url_for_get_key(client->uri, az_key_type_span, key_name, url_buffer_span));

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      az_mut_span_to_span(url_buffer_span),
      AZ_SPAN_NULL));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->retry_options.service_version));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}
