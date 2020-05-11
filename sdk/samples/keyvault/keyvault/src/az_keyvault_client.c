// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_config_internal.h>
#include <az_credentials.h>
#include <az_credentials_internal.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_json.h>
#include <az_keyvault.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg.h>

enum
{
  _az_KEYVAULT_HTTP_REQUEST_HEADER_BUF_SIZE = 10 * sizeof(az_pair),
};

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
static az_span const AZ_HTTP_HEADER_API_VERSION = AZ_SPAN_LITERAL_FROM_STR("api-version");

AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_keys()
{
  return AZ_SPAN_FROM_STR("keys");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_create()
{
  return AZ_SPAN_FROM_STR("create");
}

AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_content_type()
{
  return AZ_SPAN_FROM_STR("Content-Type");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_client_constant_for_application_json()
{
  return AZ_SPAN_FROM_STR("application/json");
}

AZ_NODISCARD az_keyvault_keys_client_options az_keyvault_keys_client_options_default()
{
  az_keyvault_keys_client_options options = (az_keyvault_keys_client_options){
    ._internal = {
      .api_version = _az_http_policy_apiversion_options_default(),
      ._telemetry_options = _az_http_policy_telemetry_options_default(),
    },
    .retry = _az_http_policy_retry_options_default(),
  };

  options._internal.api_version._internal.option_location
      = _az_http_policy_apiversion_option_location_queryparameter;
  options._internal.api_version._internal.name = AZ_HTTP_HEADER_API_VERSION;
  options._internal.api_version._internal.version = AZ_KEYVAULT_API_VERSION;

  options.retry.max_retries = 3;
  options.retry.retry_delay_msec = 1 * _az_TIME_MILLISECONDS_PER_SECOND;
  options.retry.max_retry_delay_msec = 30 * _az_TIME_MILLISECONDS_PER_SECOND;

  return options;
}

// TODO: Rename the self parameter to client to be  consistent with other clients.
AZ_NODISCARD az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client* self,
    az_span uri,
    void* credential,
    az_keyvault_keys_client_options* options)
{
  _az_precondition_not_null(self);
  _az_precondition_not_null(options);

  _az_credential* const cred = (_az_credential*)credential;

  *self = (az_keyvault_keys_client) {
    ._internal = {
      .uri = AZ_SPAN_FROM_BUFFER(self->_internal.url_buffer),
      .options = *options,
      .credential = cred,
      .pipeline = (_az_http_pipeline){
        ._internal = {
          .p_policies = {
            {
              ._internal = {
                .process = az_http_pipeline_policy_apiversion,
                .p_options= &self->_internal.options._internal.api_version,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_telemetry,
                .p_options = &self->_internal.options._internal._telemetry_options,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_retry,
                .p_options = &self->_internal.options.retry,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_credential,
                .p_options = cred,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_logging,
                .p_options = NULL,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_transport,
                .p_options= NULL,
              },
            },
          },
        }
      }
    }
  };

  // Copy url to client buffer so customer can re-use buffer on his/her side
  int32_t uri_size = az_span_size(uri);
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(self->_internal.uri, uri_size);
  az_span_copy(self->_internal.uri, uri);
  self->_internal.uri = az_span_slice(self->_internal.uri, 0, uri_size);

  AZ_RETURN_IF_FAILED(
      _az_credential_set_scopes(cred, AZ_SPAN_FROM_STR("https://vault.azure.net/.default")));

  return AZ_OK;
}

AZ_NODISCARD az_keyvault_create_key_options az_keyvault_create_key_options_default()
{
  return (az_keyvault_create_key_options){ .operations = NULL, .tags = NULL };
}

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
    az_keyvault_create_key_options* options,
    az_span* http_body);

AZ_NODISCARD az_result _az_keyvault_keys_key_create_build_json_body(
    az_json_web_key_type json_web_key_type,
    az_keyvault_create_key_options* options,
    az_span* http_body)
{
  az_json_builder builder;

  AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, *http_body, NULL));

  AZ_RETURN_IF_FAILED(az_json_builder_append_begin_object(&builder));
  // Required fields
  AZ_RETURN_IF_FAILED(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("kty")));
  AZ_RETURN_IF_FAILED(az_json_builder_append_string(&builder, json_web_key_type));

  /**************** Non-Required fields ************/
  if (options != NULL)
  {
    // Attributes
    {
      // operations
      if (options->operations != NULL)
      {
        AZ_RETURN_IF_FAILED(
            az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("key_ops")));
        AZ_RETURN_IF_FAILED(az_json_builder_append_begin_array(&builder));

        for (size_t op = 0; true; ++op)
        {
          az_span s = options->operations[op];
          if (az_span_is_content_equal(s, AZ_SPAN_NULL))
          {
            break;
          }
          AZ_RETURN_IF_FAILED(az_json_builder_append_string(&builder, s));
        }
        AZ_RETURN_IF_FAILED(az_json_builder_append_end_array(&builder));
      }
      // tags
      if (options->tags != NULL)
      {
        AZ_RETURN_IF_FAILED(
            az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("tags")));
        AZ_RETURN_IF_FAILED(az_json_builder_append_begin_object(&builder));

        for (size_t tag_index = 0; true; ++tag_index)
        {
          az_pair const tag = options->tags[tag_index];
          if (az_span_is_content_equal(tag.key, AZ_SPAN_NULL))
          {
            break;
          }
          AZ_RETURN_IF_FAILED(az_json_builder_append_property_name(&builder, tag.key));
          AZ_RETURN_IF_FAILED(az_json_builder_append_string(&builder, tag.value));
        }
        AZ_RETURN_IF_FAILED(az_json_builder_append_end_object(&builder));
      }
    }
  }

  AZ_RETURN_IF_FAILED(az_json_builder_append_end_object(&builder));
  *http_body = az_json_builder_get_json(&builder);

  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_key_create(
    az_keyvault_keys_client* client,
    az_context* context,
    az_span key_name,
    az_json_web_key_type json_web_key_type,
    az_keyvault_create_key_options* options,
    az_http_response* response)
{

  // Url buffer
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUF_SIZE];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  // copy url from client
  int32_t uri_size = az_span_size(client->_internal.uri);
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(request_url_span, uri_size);
  az_span_copy(request_url_span, client->_internal.uri);

  // Headers buffer
  uint8_t headers_buffer[_az_KEYVAULT_HTTP_REQUEST_HEADER_BUF_SIZE];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // Allocate buffer in stack to hold body request
  uint8_t body_buffer[AZ_HTTP_REQUEST_BODY_BUF_SIZE];
  az_span json_builder = AZ_SPAN_FROM_BUFFER(body_buffer);
  AZ_RETURN_IF_FAILED(
      _az_keyvault_keys_key_create_build_json_body(json_web_key_type, options, &json_builder));
  az_span const created_body = json_builder;

  // create request
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb,
      context,
      az_http_method_post(),
      request_url_span,
      uri_size,
      request_headers_span,
      created_body));

  // add path to request
  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, az_keyvault_client_constant_for_keys()));

  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, key_name));

  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, az_keyvault_client_constant_for_create()));

  // Adding header content-type json
  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      &hrb,
      az_keyvault_client_constant_for_content_type(),
      az_keyvault_client_constant_for_application_json()));

  // start pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &hrb, response);
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
    az_keyvault_keys_client* client,
    az_context* context,
    az_span key_name,
    az_span key_version,
    az_http_response* response)
{
  // create request buffer TODO: define size for a getKey Request

  uint8_t headers_buffer[_az_KEYVAULT_HTTP_REQUEST_HEADER_BUF_SIZE];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);
  // Url buffer
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUF_SIZE];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  // copy url from client
  int32_t uri_size = az_span_size(client->_internal.uri);
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(request_url_span, uri_size);
  az_span_copy(request_url_span, client->_internal.uri);

  // create request
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb,
      context,
      az_http_method_get(),
      request_url_span,
      uri_size,
      request_headers_span,
      AZ_SPAN_NULL));

  // Add path to request
  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, az_keyvault_client_constant_for_keys()));

  // Add path to request after adding query parameter
  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, key_name));

  // Add key_version if requested
  if (az_span_size(key_version) > 0)
  {
    AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, key_version));
  }

  // start pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &hrb, response);
}

AZ_NODISCARD az_result az_keyvault_keys_key_delete(
    az_keyvault_keys_client* client,
    az_context* context,
    az_span key_name,
    az_http_response* response)
{

  // Url buffer
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUF_SIZE];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  // copy url from client
  int32_t uri_size = az_span_size(client->_internal.uri);
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(request_url_span, uri_size);
  az_span_copy(request_url_span, client->_internal.uri);

  uint8_t headers_buffer[_az_KEYVAULT_HTTP_REQUEST_HEADER_BUF_SIZE];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  // TODO: define max URL size
  _az_http_request hrb;
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb,
      context,
      az_http_method_delete(),
      request_url_span,
      uri_size,
      request_headers_span,
      AZ_SPAN_NULL));

  // Add path to request
  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, az_keyvault_client_constant_for_keys()));
  AZ_RETURN_IF_FAILED(az_http_request_append_path(&hrb, key_name));

  // start pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &hrb, response);
}
