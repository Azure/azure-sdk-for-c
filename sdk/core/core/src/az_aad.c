// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_aad_private.h"
#include <az_config_internal.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_json.h>
#include <az_platform_internal.h>

#include <stddef.h>

#include <_az_cfg.h>

AZ_NODISCARD bool _az_token_expired(_az_token const* token)
{
  int64_t const expires_at_msec = token->_internal.expires_at_msec;
  return expires_at_msec <= 0 || az_platform_clock_msec() > expires_at_msec;
}

AZ_NODISCARD az_result _az_token_set(_az_token* self, _az_token const* new_token)
{
  // TODO: thread sync
  *self = *new_token;
  return AZ_OK;
}

static AZ_NODISCARD az_result
_az_span_append_with_url_encode(az_span dst, az_span src, az_span* out)
{
  int32_t dst_length = az_span_length(dst);
  uint8_t* p_dst = az_span_ptr(dst);
  int32_t remaining = az_span_capacity(dst) - dst_length;
  // get remaining from dst
  az_span span_from_current_length_to_capacity = az_span_init(p_dst + dst_length, 0, remaining);

  // Copy src into remaining with encoded (copy handles overflow)
  AZ_RETURN_IF_FAILED(az_span_copy_url_encode(
      span_from_current_length_to_capacity, src, &span_from_current_length_to_capacity));
  // return new span with updated length
  *out = az_span_init(
      az_span_ptr(dst),
      dst_length + az_span_length(span_from_current_length_to_capacity),
      az_span_capacity(dst));

  return AZ_OK;
}

// https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-auth-code-flow#request-an-access-token
AZ_NODISCARD az_result _az_aad_build_url(az_span url, az_span tenant_id, az_span* out_url)
{
  AZ_RETURN_IF_FAILED(
      az_span_append(url, AZ_SPAN_FROM_STR("https://login.microsoftonline.com/"), out_url));

  AZ_RETURN_IF_FAILED(_az_span_append_with_url_encode(*out_url, tenant_id, out_url));

  AZ_RETURN_IF_FAILED(az_span_append(*out_url, AZ_SPAN_FROM_STR("/oauth2/v2.0/token"), out_url));

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

  AZ_RETURN_IF_FAILED(
      az_span_append(body, AZ_SPAN_FROM_STR("grant_type=client_credentials&client_id="), out_body));
  AZ_RETURN_IF_FAILED(_az_span_append_with_url_encode(*out_body, client_id, out_body));

  AZ_RETURN_IF_FAILED(az_span_append(*out_body, AZ_SPAN_FROM_STR("&scope="), out_body));
  AZ_RETURN_IF_FAILED(_az_span_append_with_url_encode(*out_body, scopes, out_body));

  if (az_span_length(client_secret) > 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*out_body, AZ_SPAN_FROM_STR("&client_secret="), out_body));
    AZ_RETURN_IF_FAILED(_az_span_append_with_url_encode(*out_body, client_secret, out_body));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_aad_request_token(_az_http_request* ref_request, _az_token* out_token)
{
  // FIXME: If you uncomment the line below, we'll start getting HTTP 400 Bad Request instead of 200
  // OK. I suspect, it is because there's a bug in the code that adds headers. Could be something
  // else, of course.
  /*AZ_RETURN_IF_FAILED(az_http_request_append_header(
      ref_request,
      AZ_SPAN_FROM_STR("Content-Type"),
      AZ_SPAN_FROM_STR("application/x-www-url-form-urlencoded")));*/

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
      .p_policies = {
        {._internal = { .process = az_http_pipeline_policy_retry, .p_options = &retry_options, }, },
        {._internal = { .process = az_http_pipeline_policy_logging, .p_options = NULL, }, },
        {._internal = { .process = az_http_pipeline_policy_transport, .p_options = NULL, }, },
      },
    },
  };

  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, ref_request, &response));

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

  // Expiration
  az_json_token json_token;
  AZ_RETURN_IF_FAILED(az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/expires_in"), &json_token));

  double expires_in_seconds = 0;
  AZ_RETURN_IF_FAILED(az_json_token_get_number(&json_token, &expires_in_seconds));

  // We'll assume the token expires 3 minutes prior to its actual expiration.
  int64_t const expires_in_msec
      = (((int64_t)expires_in_seconds) - (3 * _az_TIME_SECONDS_PER_MINUTE))
      * _az_TIME_MILLISECONDS_PER_SECOND;

  // Access token
  AZ_RETURN_IF_FAILED(
      az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/access_token"), &json_token));

  az_span access_token = { 0 };
  AZ_RETURN_IF_FAILED(az_json_token_get_string(&json_token, &access_token));

  _az_token new_token = {
    ._internal = {
      .token = { 0 },
      .token_length = 0,
      .expires_at_msec = az_platform_clock_msec() + expires_in_msec,
    },
  };

  az_span new_token_span = AZ_SPAN_FROM_BUFFER(new_token._internal.token);
  AZ_RETURN_IF_FAILED(az_span_copy(new_token_span, AZ_SPAN_FROM_STR("Bearer "), &new_token_span));
  AZ_RETURN_IF_FAILED(az_span_append(new_token_span, access_token, &new_token_span));

  new_token._internal.token_length = (int16_t)az_span_length(new_token_span);

  AZ_RETURN_IF_FAILED(_az_token_set(out_token, &new_token));

  return AZ_OK;
}
