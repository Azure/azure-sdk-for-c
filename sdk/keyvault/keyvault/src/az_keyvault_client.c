// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_json.h>
#include <az_keyvault.h>

#include <az_span_internal.h>

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

AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_keys() {
  return AZ_SPAN_FROM_STR("keys");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_create() {
  return AZ_SPAN_FROM_STR("create");
}

AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_content_type() {
  return AZ_SPAN_FROM_STR("Content-Type");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_application_json() {
  return AZ_SPAN_FROM_STR("application/json");
}

az_keyvault_keys_client_options const AZ_KEYVAULT_CLIENT_DEFAULT_OPTIONS
    = { .service_version = AZ_SPAN_LITERAL_FROM_STR("7.0"),
        .retry = {
            .max_retry = 3,
            .delay_in_ms = 30,
        } };

/**
 * @brief Internal inline function in charge of building json payload for creating a new key
 *
 * @param json_web_key_type type of the key. It will be always added to json payload
 * @param options all optional settings that can be inside create key options
 * @param http_body action used by json builder to be called while building
 * @return AZ_NODISCARD _az_keyvault_keys_key_create_build_json_body
 */
AZ_NODISCARD az_result _az_keyvault_keys_key_create_build_json_body(
    az_span json_web_key_type,
    az_keyvault_create_key_options * options,
    az_span http_body) {

  az_json_builder builder = { 0 };

  AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, http_body));

  AZ_RETURN_IF_FAILED(az_json_builder_append_token(&builder, az_json_token_object()));
  // Required fields
  AZ_RETURN_IF_FAILED(az_json_builder_append_object(
      &builder, AZ_SPAN_FROM_STR("kty"), az_json_token_string(json_web_key_type)));

  /**************** Non-Required fields ************/
  if (options != NULL) {
    // Attributes
    {
      az_optional_bool const enabled_field = options->enabled;
      if (enabled_field.is_present) {
        AZ_RETURN_IF_FAILED(az_json_builder_append_object(
            &builder, AZ_SPAN_FROM_STR("attributes"), az_json_token_object()));
        AZ_RETURN_IF_FAILED(az_json_builder_append_object(
            &builder, AZ_SPAN_FROM_STR("enabled"), az_json_token_boolean(enabled_field.data)));
        AZ_RETURN_IF_FAILED(az_json_builder_append_object_close(&builder));
      }
      // operations
      if (options->operations != NULL) {
        AZ_RETURN_IF_FAILED(az_json_builder_append_object(
            &builder, AZ_SPAN_FROM_STR("key_ops"), az_json_token_array()));
        for (size_t op = 0; true; ++op) {
          az_span s = options->operations[op];
          if (az_span_is_equal(s, az_span_null())) {
            break;
          }
          AZ_RETURN_IF_FAILED(az_json_builder_append_array_item(&builder, az_json_token_string(s)));
        }
        AZ_RETURN_IF_FAILED(az_json_builder_append_array_close(&builder));
      }
      // tags
      if (options->tags != NULL) {
        AZ_RETURN_IF_FAILED(az_json_builder_append_object(
            &builder, AZ_SPAN_FROM_STR("tags"), az_json_token_object()));
        for (size_t tag_index = 0; true; ++tag_index) {
          az_pair const tag = options->tags[tag_index];
          if (az_span_is_equal(tag.key, az_span_null())) {
            break;
          }
          AZ_RETURN_IF_FAILED(
              az_json_builder_append_object(&builder, tag.key, az_json_token_string(tag.value)));
        }
        AZ_RETURN_IF_FAILED(az_json_builder_append_object_close(&builder));
      }
    }
  }

  AZ_RETURN_IF_FAILED(az_json_builder_append_object_close(&builder));

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
  az_span request_buffer_span = AZ_SPAN_FROM_BUFFER(request_buffer);

  /* ******** build url for request  ******/

  // Allocate buffer in stack to hold body request
  uint8_t body_buffer[MAX_BODY_SIZE];
  az_span json_builder = AZ_SPAN_FROM_BUFFER(body_buffer);
  AZ_RETURN_IF_FAILED(
      _az_keyvault_keys_key_create_build_json_body(json_web_key_type, options, json_builder));
  az_span const created_body = json_builder;

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
  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_path(&hrb, az_keyvault_client_constant_for_keys()));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_SPAN_FROM_STR("api-version"), client->retry_options.service_version));

  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_name));

  // add extra header just for testing append_path after another query
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_SPAN_FROM_STR("ignore"), client->retry_options.service_version));

  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_path(&hrb, az_keyvault_client_constant_for_create()));

  // Adding header content-type json
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      &hrb,
      az_keyvault_client_constant_for_content_type(),
      az_keyvault_client_constant_for_application_json()));

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
  az_span request_buffer_span = AZ_SPAN_FROM_BUFFER(request_buffer);

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      client->uri,
      az_span_null()));

  // Add path to request
  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_path(&hrb, az_keyvault_client_constant_for_keys()));

  // add version to request as query parameter
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_SPAN_FROM_STR("api-version"), client->retry_options.service_version));

  // Add path to request after adding query parameter
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_name));

  // Add version if requested
  if (az_span_length(key_version) > 0) {
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
  az_span request_buffer_span = AZ_SPAN_FROM_BUFFER(request_buffer);

  // create request
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_DELETE,
      client->uri,
      az_span_null()));

  // add version to request
  AZ_RETURN_IF_FAILED(az_http_request_builder_set_query_parameter(
      &hrb, AZ_SPAN_FROM_STR("api-version"), client->retry_options.service_version));

  // Add path to request
  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_path(&hrb, az_keyvault_client_constant_for_keys()));
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_path(&hrb, key_name));

  // start pipeline
  return az_http_pipeline_process(&client->pipeline, &hrb, response);
}
