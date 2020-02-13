// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_aad_private.h"
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>

#include <stddef.h>

#include <_az_cfg.h>

static AZ_NODISCARD az_result
_az_client_secret_credential_request_token(az_client_secret_credential * credential) {
  uint8_t url_buf[_az_AAD_REQUEST_URL_BUF_SIZE] = { 0 };
  az_span url = AZ_SPAN_FROM_BUFFER(url_buf);
  AZ_RETURN_IF_FAILED(_az_aad_build_url(url, credential->_internal.tenant_id, &url));

  uint8_t body_buf[_az_AAD_REQUEST_BODY_BUF_SIZE] = { 0 };
  az_span body = AZ_SPAN_FROM_BUFFER(body_buf);
  AZ_RETURN_IF_FAILED(_az_aad_build_body(
      body,
      credential->_internal.client_id,
      credential->_internal.scopes,
      credential->_internal.client_secret,
      &body));

  uint8_t header_buf[_az_AAD_REQUEST_HEADER_BUF_SIZE];
  _az_http_request request = { 0 };
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &request, az_http_method_post(), url, AZ_SPAN_FROM_BUFFER(header_buf), body));

  return _az_aad_request_token(
      &credential->_internal.credential._internal.http_transport_options,
      &request,
      &credential->_internal.token);
}

// This gets called from the http credential policy
static AZ_NODISCARD az_result _az_client_secret_credential_apply(
    az_client_secret_credential * credential,
    _az_http_request * ref_request) {

  if (_az_token_expired(&(credential->_internal.token))) {
    AZ_RETURN_IF_FAILED(_az_client_secret_credential_request_token(credential));
  }

  int16_t const token_length = credential->_internal.token._internal.token_length;

  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      ref_request,
      AZ_SPAN_FROM_STR("authorization"),
      az_span_init(credential->_internal.token._internal.token, token_length, token_length)));

  return AZ_OK;
}

static AZ_NODISCARD az_result
_az_client_secret_credential_set_scopes(az_client_secret_credential * self, az_span scopes) {
  self->_internal.scopes = scopes;
  return AZ_OK;
}

AZ_NODISCARD az_result az_client_secret_credential_init(
    az_client_secret_credential * self,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret) {
  *self = (az_client_secret_credential){
    ._internal = {
      .credential = {
        ._internal = {
          .http_transport_options = NULL,
          .apply_credential = (_az_credential_apply_fn)_az_client_secret_credential_apply,
          .set_scopes = (_az_credential_set_scopes_fn)_az_client_secret_credential_set_scopes,
          },
        },
        .tenant_id = tenant_id,
        .client_id = client_id,
        .client_secret = client_secret,
        .scopes = { 0 },
        .token = { 0 }
      },
    };

  return AZ_OK;
}
