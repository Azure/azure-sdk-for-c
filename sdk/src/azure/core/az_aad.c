// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_aad_private.h"
#include "az_credential_token_private.h"
#include <azure/core/az_http.h>
#include <azure/core/az_json.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_http_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <stddef.h>

#include <azure/core/_az_cfg.h>

// https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-auth-code-flow#request-an-access-token
AZ_NODISCARD az_result _az_aad_build_url(az_span url, az_span tenant_id, az_span* out_url)
{
  az_span const root_url = AZ_SPAN_FROM_STR("https://login.microsoftonline.com/");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(url, az_span_size(root_url));
  az_span remainder = az_span_copy(url, root_url);

  {
    int32_t url_length = 0;
    AZ_RETURN_IF_FAILED(_az_span_url_encode(remainder, tenant_id, &url_length));
    remainder = az_span_slice_to_end(remainder, url_length);
  }

  az_span const oath_token = AZ_SPAN_FROM_STR("/oauth2/v2.0/token");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, az_span_size(oath_token));
  remainder = az_span_copy(remainder, oath_token);

  *out_url = az_span_slice(url, 0, _az_span_diff(remainder, url));

  return AZ_OK;
}

// https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-auth-code-flow#request-an-access-token
AZ_NODISCARD az_result _az_aad_build_body(
    az_span body,
    az_span client_id,
    az_span scopes,
    az_span client_secret,
    az_span* out_body)
{
  az_span const grant_type_and_client_id_key
      = AZ_SPAN_FROM_STR("grant_type=client_credentials&client_id=");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(body, az_span_size(grant_type_and_client_id_key));

  az_span remainder = az_span_copy(body, grant_type_and_client_id_key);
  int32_t url_length = 0;

  AZ_RETURN_IF_FAILED(_az_span_url_encode(remainder, client_id, &url_length));
  remainder = az_span_slice_to_end(remainder, url_length);

  az_span const scope_key = AZ_SPAN_FROM_STR("&scope=");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, az_span_size(scope_key));

  remainder = az_span_copy(remainder, scope_key);

  AZ_RETURN_IF_FAILED(_az_span_url_encode(remainder, scopes, &url_length));
  remainder = az_span_slice_to_end(remainder, url_length);

  if (az_span_size(client_secret) > 0)
  {
    az_span const client_secret_key = AZ_SPAN_FROM_STR("&client_secret=");
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, az_span_size(client_secret_key));

    remainder = az_span_copy(remainder, client_secret_key);

    AZ_RETURN_IF_FAILED(_az_span_url_encode(remainder, client_secret, &url_length));
    remainder = az_span_slice_to_end(remainder, url_length);
  }

  *out_body = az_span_slice(body, 0, _az_span_diff(remainder, body));
  return AZ_OK;
}

AZ_NODISCARD static az_result _az_parse_json_payload(
    az_span body,
    int64_t* expires_in_seconds,
    az_json_token* json_access_token)
{
  az_json_reader jr;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, body, NULL));

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  bool found_expires_in = false;
  bool found_access_token = false;

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("expires_in")))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      AZ_RETURN_IF_FAILED(az_json_token_get_int64(&jr.token, expires_in_seconds));
      found_expires_in = true;
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("access_token")))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      *json_access_token = jr.token;
      found_access_token = true;
    }
    else
    {
      // ignore other tokens
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jr));
    }
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  }

  if (!found_expires_in || !found_access_token)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_aad_request_token(_az_http_request* request, _az_token* out_token)
{
  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      request,
      AZ_SPAN_FROM_STR("Content-Type"),
      AZ_SPAN_FROM_STR("application/x-www-form-urlencoded")));

  uint8_t response_buf[_az_AAD_RESPONSE_BUF_SIZE] = { 0 };
  az_http_response response = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buf)));

  az_http_policy_retry_options retry_options = _az_http_policy_retry_options_default();
  retry_options.max_retries = 7;
  retry_options.retry_delay_msec = 1 * _az_TIME_MILLISECONDS_PER_SECOND;
  retry_options.max_retry_delay_msec
      = 1 * _az_TIME_SECONDS_PER_MINUTE * _az_TIME_MILLISECONDS_PER_SECOND;

  // Make a HTTP request to get token
  _az_http_pipeline pipeline = (_az_http_pipeline){
    ._internal = {
      .policies = {
        {._internal = { .process = az_http_pipeline_policy_retry, .options = &retry_options, }, },
#ifndef AZ_NO_LOGGING
        {._internal = { .process = az_http_pipeline_policy_logging, .options = NULL, }, },
#endif // AZ_NO_LOGGING
        {._internal = { .process = az_http_pipeline_policy_transport, .options = NULL, }, },
      },
    },
  };

  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, request, &response));

  // If we failed to get the token, we return failure/
  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_status_line(&response, &status_line));
  if (status_line.status_code != AZ_HTTP_STATUS_CODE_OK)
  {
    return AZ_ERROR_HTTP_AUTHENTICATION_FAILED;
  }

  // We successfully got the token, let's parse the body.
  az_span body = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_body(&response, &body));

  int64_t expires_in_seconds = 0;
  az_json_token json_access_token = { 0 };
  AZ_RETURN_IF_FAILED(_az_parse_json_payload(body, &expires_in_seconds, &json_access_token));

  // We'll assume the token expires 3 minutes prior to its actual expiration.
  int64_t const expires_in_msec = ((expires_in_seconds) - (3 * _az_TIME_SECONDS_PER_MINUTE))
      * _az_TIME_MILLISECONDS_PER_SECOND;

  _az_token new_token = {
    ._internal = {
      .token = { 0 },
      .token_length = 0,
      .expires_at_msec = az_platform_clock_msec() + expires_in_msec,
    },
  };

  az_span new_token_span = AZ_SPAN_FROM_BUFFER(new_token._internal.token);
  az_span remainder = az_span_copy(new_token_span, AZ_SPAN_FROM_STR("Bearer "));

  int32_t bytes_written = 0;
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &json_access_token, (char*)az_span_ptr(remainder), az_span_size(remainder), &bytes_written));

  new_token._internal.token_length
      = (int16_t)(_az_span_diff(remainder, new_token_span) + bytes_written);

  *out_token = new_token;

  return AZ_OK;
}
