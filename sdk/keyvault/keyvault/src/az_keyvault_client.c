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

static az_span const AZ_KEYVAULT_KEY_TYPE_KEY_STR = AZ_CONST_STR("keys");
// static az_span const AZ_KEYVAULT_KEY_TYPE_SECRET_STR = AZ_CONST_STR("secrets");
// static az_span const AZ_KEYVAULT_KEY_TYPE_CERTIFICATE_STR = AZ_CONST_STR("certificates");

az_span const AZ_KEYVAULT_WEB_KEY_TYPE_EC_STR = AZ_CONST_STR("EC");
az_span const AZ_KEYVAULT_WEB_KEY_TYPE_EC_HSM_STR = AZ_CONST_STR("EC-HSM");
az_span const AZ_KEYVAULT_WEB_KEY_TYPE_RSA_STR = AZ_CONST_STR("RSA");
az_span const AZ_KEYVAULT_WEB_KEY_TYPE_RSA_HSM_STR = AZ_CONST_STR("RSA-HSM");
az_span const AZ_KEYVAULT_WEB_KEY_TYPE_OCT_STR = AZ_CONST_STR("oct");

static az_span const AZ_KEYVAULT_REST_KEY_URL_KEYS = AZ_CONST_STR("keys");
static az_span const AZ_KEYVAULT_CREATE_KEY_URL_CREATE = AZ_CONST_STR("create");

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

/**
 * @brief Action that uses json builder to construct http request body used by create key
 *
 * @param kty required value to create a new key
 * @param write
 * @return AZ_NODISCARD build_request_json_body
 */
AZ_NODISCARD AZ_INLINE az_result build_request_json_body(
    az_span const json_web_key_type,
    az_keyvault_create_key_options const * const options,
    az_span_action const write) {

  az_json_builder builder = { 0 };

  AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, write));

  AZ_RETURN_IF_FAILED(az_json_builder_write(&builder, az_json_value_create_object()));
  // Required fields
  AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
      &builder, AZ_STR("kty"), az_json_value_create_string(json_web_key_type)));

  /**************** Non-Required fields ************/
  if (options != NULL) {
    // Attributes
    {
      az_optional_bool const enabled_field = options->enabled;
      if (enabled_field.is_present) {
        AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
            &builder, AZ_STR("attributes"), az_json_value_create_object()));
        AZ_RETURN_IF_FAILED(az_json_builder_write_object_member(
            &builder, AZ_STR("enabled"), az_json_value_create_boolean(enabled_field.data)));
        AZ_RETURN_IF_FAILED(az_json_builder_write_object_close(&builder));
      }
    }
  }

  AZ_RETURN_IF_FAILED(az_json_builder_write_object_close(&builder));

  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_span const json_web_key_type,
    az_keyvault_create_key_options * const options,
    az_http_response * const response) {

  // Request buffer
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  /* ******** build url for request  ******/

  // Allocate buffer in stack to hold body request
  uint8_t body_buffer[MAX_BODY_SIZE];
  az_span_builder json_builder
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(body_buffer));
  AZ_RETURN_IF_FAILED(build_request_json_body(
      json_web_key_type, options, az_span_builder_append_action(&json_builder)));
  az_span const created_body = az_span_builder_result(&json_builder);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_POST,
      client->uri,
      created_body));

  // add path to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, AZ_KEYVAULT_REST_KEY_URL_KEYS));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->retry_options.service_version));

  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_name));

  // add extra header just for testing append_path after another query
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("ignore"), client->retry_options.service_version));

  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, AZ_KEYVAULT_CREATE_KEY_URL_CREATE));

  // Adding header content-type json
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb,
      AZ_HTTP_REQUEST_BUILDER_HEADER_CONTENT_TYPE_LABEL,
      AZ_HTTP_REQUEST_BUILDER_HEADER_CONTENT_TYPE_JSON));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
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
    az_span const key_version,
    az_http_response * const response) {
  // create request buffer TODO: define size for a getKey Request
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      client->uri,
      az_span_create_empty()));

  // Add path to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, AZ_KEYVAULT_KEY_TYPE_KEY_STR));

  // add version to request as query parameter
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->retry_options.service_version));

  // Add path to request after adding query parameter
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_name));

  // Add version if requested
  if (!az_span_is_empty(key_version)) {
    AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_version));
  }

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}

AZ_NODISCARD az_result az_keyvault_keys_key_delete(
    az_keyvault_keys_client * client,
    az_span const key_name,
    az_http_response * const response) {
  // Request buffer
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_DELETE,
      client->uri,
      az_span_create_empty()));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_STR("api-version"), client->retry_options.service_version));

  // Add path to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, AZ_KEYVAULT_REST_KEY_URL_KEYS));
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_name));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}
