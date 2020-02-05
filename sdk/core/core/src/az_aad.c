// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_aad_private.h"
#include <az_http_pipeline_internal.h>
#include <az_json.h>
#include <az_time_internal.h>

AZ_NODISCARD bool _az_token_expired(_az_token const * token) {
  int64_t const expires_at_msec = token->_internal.expires_at_msec;
  return expires_at_msec <= 0 || _az_pal_clock_msec() > expires_at_msec;
}

AZ_NODISCARD _az_token _az_token_get(_az_token const * self) {
  // TODO: thread sync
  _az_token token = *self;
  return token;
}

AZ_NODISCARD az_result _az_token_set(_az_token * self, _az_token const * new_token) {
  // TODO: thread sync
  *self = *new_token;
  return AZ_OK;
}

// https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-auth-code-flow#request-an-access-token
AZ_NODISCARD az_result _az_aad_build_url(az_span url, az_span tenant_id, az_span * out_url) {
  AZ_RETURN_IF_FAILED(
      az_span_append(url, AZ_SPAN_FROM_STR("https://login.microsoftonline.com/"), out_url));

  AZ_RETURN_IF_FAILED(az_uri_encode(tenant_id, out_url));
  AZ_RETURN_IF_FAILED(az_span_append(*out_url, AZ_SPAN_FROM_STR("/oauth2/v2.0/token"), out_url));

  return AZ_OK;
}

// https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-auth-code-flow#request-an-access-token
AZ_NODISCARD az_result _az_aad_build_body(
    az_span body,
    az_span client_id,
    az_span scopes,
    az_span client_secret,
    az_span * out_body) {

  AZ_RETURN_IF_FAILED(
      az_span_append(body, AZ_SPAN_FROM_STR("grant_type=client_credentials&client_id="), out_body));
  AZ_RETURN_IF_FAILED(az_url_encode(client_id, out_body));

  AZ_RETURN_IF_FAILED(az_span_append(*out_body, AZ_SPAN_FROM_STR("&scope="), out_body));
  AZ_RETURN_IF_FAILED(az_uri_encode(scopes, out_body));

  if (az_span_length(client_secret) > 0) {
    AZ_RETURN_IF_FAILED(az_span_append(*out_body, AZ_SPAN_FROM_STR("&client_secret="), out_body));
    AZ_RETURN_IF_FAILED(az_uri_encode(client_secret, out_body));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_aad_request_token(az_http_request * ref_request, _az_token * out_token) {
  az_http_request_append_header(
      &ref_request,
      AZ_SPAN_FROM_STR("Content-Type"),
      AZ_SPAN_FROM_STR("application/x-www-url-form-urlencoded"));

  uint8_t response_buf[_az_AAD_RESPONSE_BUF_SIZE] = { 0 };
  az_http_response response = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buf)));

  // Make a HTTP request to get token
  az_http_pipeline pipeline = {
      .policies = {
        { .process = az_http_pipeline_policy_retry, .data = NULL },
        { .process = az_http_pipeline_policy_logging, .data = NULL },
        { .process = az_http_pipeline_policy_transport, .data = NULL },
      },
    };
  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, &ref_request, &response));

  // If we failed to get the token, we return failure/
  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_status_line(&response, &status_line));
  if (status_line.status_code != AZ_HTTP_STATUS_CODE_OK) {
    return AZ_ERROR_HTTP_AUTHENTICATION_FAILED;
  }

  // We successfully got the token, let's parse the body.
  az_span body = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_body(&response, &body));

  // Expiration
  az_json_token json_token;
  AZ_RETURN_IF_FAILED(az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/expires_in"), &json_token));

  double expires_in_seconds = 0;
  AZ_RETURN_IF_FAILED(az_json_token_get_number(json_token, &expires_in_seconds));

  // We'll assume the token expires 3 minutes prior to its actual expiration.
  int64_t const expires_in_msec
      = (((int64_t)expires_in_seconds) - (3 * _az_TIME_SECONDS_PER_MINUTE))
      * _az_TIME_MILLISECONDS_PER_SECOND;

  // Access token
  AZ_RETURN_IF_FAILED(
      az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/access_token"), &json_token));

  az_span access_token = { 0 };
  AZ_RETURN_IF_FAILED(az_json_token_get_string(json_token, &access_token));

  _az_token new_token = {
    ._internal = {
      .token = { 0 },
      .token_length = 0,
      .expires_at_msec = _az_pal_clock_msec() + expires_in_msec,
    },
  };

  az_span new_token_span = AZ_SPAN_FROM_BUFFER(new_token._internal.token);
  AZ_RETURN_IF_FAILED(az_span_copy(new_token_span, AZ_SPAN_FROM_STR("Bearer "), &new_token_span));
  AZ_RETURN_IF_FAILED(az_span_append(new_token_span, access_token, &new_token_span));

  new_token._internal.token_length = az_span_length(new_token_span);

  _az_token_set(out_token, &new_token);

  return AZ_OK;
}
