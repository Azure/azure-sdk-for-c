// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_aad_private.h"
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>

#include <stddef.h>

#include <_az_cfg.h>

static AZ_NODISCARD az_result _az_credential_client_secret_request_token(
    az_credential_client_secret* credential,
    az_context* context)
{
  uint8_t url_buf[_az_AAD_REQUEST_URL_BUF_SIZE] = { 0 };
  az_span url_span = AZ_SPAN_FROM_BUFFER(url_buf);
  az_span url;
  AZ_RETURN_IF_FAILED(_az_aad_build_url(url_span, credential->_internal.tenant_id, &url));

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
      &request,
      context,
      az_http_method_post(),
      url_span,
      az_span_size(url),
      AZ_SPAN_FROM_BUFFER(header_buf),
      body));

  return _az_aad_request_token(&request, &credential->_internal.token);
}

az_span const _az_auth_header_name = AZ_SPAN_LITERAL_FROM_STR("authorization");

// This gets called from the http credential policy
static AZ_NODISCARD az_result _az_credential_client_secret_apply(
    az_credential_client_secret* credential,
    _az_http_request* ref_request)
{

  if (_az_token_expired(&(credential->_internal.token)))
  {
    AZ_RETURN_IF_FAILED(
        _az_credential_client_secret_request_token(credential, ref_request->_internal.context));
  }

  int16_t const token_length = credential->_internal.token._internal.token_length;

  AZ_RETURN_IF_FAILED(az_http_request_append_header(
      ref_request,
      _az_auth_header_name,
      az_span_init(credential->_internal.token._internal.token, token_length)));

  return AZ_OK;
}

static AZ_NODISCARD az_result
_az_credential_client_secret_set_scopes(az_credential_client_secret* self, az_span scopes)
{
  self->_internal.scopes = scopes;
  return AZ_OK;
}

AZ_NODISCARD az_result az_credential_client_secret_init(
    az_credential_client_secret* self,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret)
{
  *self = (az_credential_client_secret){
    ._internal = {
      .credential = {
        ._internal = {
          .apply_credential = (_az_credential_apply_fn)_az_credential_client_secret_apply,
          .set_scopes = (_az_credential_set_scopes_fn)_az_credential_client_secret_set_scopes,
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
