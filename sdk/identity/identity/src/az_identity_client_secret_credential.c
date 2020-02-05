// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_identity_client_secret_credential.h>

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_identity_access_token_context.h>
#include <az_identity_credential.h>
#include <az_json.h>

#include <_az_cfg.h>

enum {
  _az_IDENTITY_CLIENT_SECRET_CREDENTIAL_RESPONSE_BUF_SIZE = 5 * (1024 / 2),
  _az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_URL_BUF_SIZE = 100,
  _az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_BODY_BUF_SIZE = 200,
  _az_IDENTITY_CLIENT_SECRET_CREDENTIAL_HRB_BUF_SIZE
  = (8
     * ((_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_URL_BUF_SIZE / 8)
        + ((_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_URL_BUF_SIZE % 8) == 0 ? 0 : 1)))
      + (1 * sizeof(az_pair)), // to fit auth URL with alignment, and a minimum amount of headers
                               // (1) added by the pipeline_process
};

AZ_NODISCARD static az_result _az_span_append_with_url_encode(
    az_span dst,
    az_span src,
    az_span * out) {
  az_span span_from_current_length_to_capacity = { 0 };
  int32_t dst_length = az_span_length(dst);
  // get remaining from dst
  AZ_RETURN_IF_FAILED(az_span_slice(dst, dst_length, -1, &span_from_current_length_to_capacity));
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

AZ_INLINE AZ_NODISCARD az_result
_az_identity_client_secret_credential_ms_oauth2_send_get_token_request(
    az_identity_access_token_context const * const token_context,
    az_span auth_url_buf,
    az_span auth_body_buf,
    az_span hrb_buf,
    az_http_response * response,
    clock_t * const requested_at) {
  (void)hrb_buf; // TODO: after refactoring HTTP request, we need to refactor this span as probably
                 // we don't need
  AZ_CONTRACT_ARG_NOT_NULL(token_context->_internal.credential);
  az_identity_client_secret_credential const * const credential
      = (az_identity_client_secret_credential const *)(token_context->_internal.credential);

  {
    AZ_RETURN_IF_FAILED(az_span_append(
        auth_url_buf, AZ_SPAN_FROM_STR("https://login.microsoftonline.com/"), &auth_url_buf));

    AZ_RETURN_IF_FAILED(
        _az_span_append_with_url_encode(auth_url_buf, credential->tenant_id, &auth_url_buf));

    AZ_RETURN_IF_FAILED(
        az_span_append(auth_url_buf, AZ_SPAN_FROM_STR("/oauth2/v2.0/token"), &auth_url_buf));
  }
  {
    AZ_RETURN_IF_FAILED(az_span_append(
        auth_body_buf,
        AZ_SPAN_FROM_STR("grant_type=client_credentials&client_id="),
        &auth_body_buf));

    AZ_RETURN_IF_FAILED(
        _az_span_append_with_url_encode(auth_body_buf, credential->client_id, &auth_body_buf));

    AZ_RETURN_IF_FAILED(
        az_span_append(auth_body_buf, AZ_SPAN_FROM_STR("&client_secret="), &auth_body_buf));

    AZ_RETURN_IF_FAILED(
        _az_span_append_with_url_encode(auth_body_buf, credential->client_secret, &auth_body_buf));

    AZ_RETURN_IF_FAILED(az_span_append(auth_body_buf, AZ_SPAN_FROM_STR("&scope="), &auth_body_buf));

    AZ_RETURN_IF_FAILED(_az_span_append_with_url_encode(
        auth_body_buf, token_context->_internal.scope, &auth_body_buf));
    // don't forget to update length
  }

  az_http_request hrb = { 0 };
  // Adding small buffer for 2 headers in case pipeline  needs to add a header or 2
  uint8_t header_buff[2 * sizeof(az_span)];
  AZ_RETURN_IF_FAILED(az_http_request_init(
      &hrb, az_http_method_post(), auth_url_buf, AZ_SPAN_FROM_BUFFER(header_buff), auth_body_buf));

  static az_http_pipeline pipeline = {
      .p_policies = {
        { .process = az_http_pipeline_policy_uniquerequestid, .p_options = NULL },
        { .process = az_http_pipeline_policy_retry, .p_options = NULL },
        { .process = az_http_pipeline_policy_logging, .p_options = NULL },
        { .process = az_http_pipeline_policy_bufferresponse, .p_options = NULL },
        { .process = az_http_pipeline_policy_distributedtracing, .p_options = NULL },
        { .process = az_http_pipeline_policy_transport, .p_options = NULL }, // TODO: We need to provide transport implementation here
      },
    };

  *requested_at = clock();
  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, &hrb, response));

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_ms_oauth2_get_token(
    az_identity_access_token_context const * const token_context,
    az_http_response * response) {
  clock_t requested_at = 0;
  {
    uint8_t auth_url_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_URL_BUF_SIZE] = { 0 };
    uint8_t auth_body_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_BODY_BUF_SIZE] = { 0 };
    uint8_t hrb_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_HRB_BUF_SIZE] = { 0 };

    az_span const auth_url = AZ_SPAN_FROM_BUFFER(auth_url_buf);
    az_span const auth_body = AZ_SPAN_FROM_BUFFER(auth_body_buf);
    az_span const hrb_buf_span = AZ_SPAN_FROM_BUFFER(hrb_buf);

    az_result const token_request_result
        = _az_identity_client_secret_credential_ms_oauth2_send_get_token_request(
            token_context, auth_url, auth_body, hrb_buf_span, response, &requested_at);

    az_span_set(hrb_buf_span, '#');
    az_span_set(auth_body, '#');
    az_span_set(auth_url, '#');
    AZ_RETURN_IF_FAILED(token_request_result);
  }

  az_span body = { 0 };
  {
    az_http_response_status_line status_line = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_get_status_line(response, &status_line));
    if (status_line.status_code != AZ_HTTP_STATUS_CODE_OK) {
      return AZ_ERROR_HTTP_PAL;
    }
    AZ_RETURN_IF_FAILED(az_http_response_get_body(response, &body));
  }

  {
    az_json_token token;

    clock_t expiration_clock = 0;
    if (requested_at > 0) {
      double expiration_seconds = 0;
      if (az_succeeded(az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/expires_in"), &token))
          && az_succeeded(az_json_token_get_number(token, &expiration_seconds))) {
        if (expiration_seconds > 0) {
          expiration_clock = (((clock_t)expiration_seconds) - (3 * 60)) * CLOCKS_PER_SEC;
        }
      }
    }

    az_span token_str = { 0 };
    AZ_RETURN_IF_FAILED(az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/access_token"), &token));
    AZ_RETURN_IF_FAILED(az_json_token_get_string(token, &token_str));

    az_span token_buf = AZ_SPAN_FROM_BUFFER(token_context->_internal.token->_internal.token_buf);

    az_span_set(token_buf, 0);

    AZ_RETURN_IF_FAILED(az_span_append(token_buf, AZ_SPAN_FROM_STR("Bearer "), &token_buf));
    az_result const token_append_result = az_span_append(token_buf, token_str, &token_buf);

    if (az_succeeded(token_append_result)) {
      token_context->_internal.token->_internal.token_size = az_span_length(token_buf);

      clock_t const expiration = requested_at + expiration_clock;
      if (expiration > 0) {
        token_context->_internal.token->_internal.token_expiration = expiration;
      }
    } else {
      az_span_set(token_buf, 'X');
      return token_append_result;
    }
  }

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_renew_token_credential(
    az_identity_access_token_context const * const token_context) {
  uint8_t response_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_RESPONSE_BUF_SIZE] = { 0 };

  az_http_response http_response = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buf)));

  az_result const result
      = _az_identity_client_secret_credential_ms_oauth2_get_token(token_context, &http_response);

  az_span_set(http_response._internal.http_response, '#');
  return result;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_ensure_token_credential(
    az_identity_access_token_context const * const token_context) {
  clock_t const expiration = token_context->_internal.token->_internal.token_expiration;
  if (expiration > 0) {
    clock_t const clk = clock();
    if (clk > 0 && clk < expiration) {
      return AZ_OK;
    }
  }

  return _az_identity_client_secret_credential_renew_token_credential(token_context);
}

static AZ_NODISCARD az_result _az_identity_client_secret_credential_credential_func(
    az_identity_access_token_context const * const token_context,
    az_http_request * const hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(token_context);
  AZ_CONTRACT_ARG_NOT_NULL(token_context->_internal.token);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);

  // Assume https without validating. I/O request will fail if not

  AZ_RETURN_IF_FAILED(_az_identity_client_secret_credential_ensure_token_credential(token_context));
  int32_t token_length = token_context->_internal.token->_internal.token_size;
  return az_http_request_append_header(
      hrb,
      AZ_SPAN_FROM_STR("authorization"),
      az_span_init(
          token_context->_internal.token->_internal.token_buf, token_length, token_length));
}

AZ_NODISCARD az_result az_identity_client_secret_credential_init(
    az_identity_client_secret_credential * const self,
    az_span const tenant_id,
    az_span const client_id,
    az_span const client_secret) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_VALID_SPAN(tenant_id);
  AZ_CONTRACT_ARG_VALID_SPAN(client_id);
  AZ_CONTRACT_ARG_VALID_SPAN(client_secret);

  *self = (az_identity_client_secret_credential){
    ._internal = {
      .credential = { 0 },
    },
    .tenant_id = tenant_id,
    .client_id = client_id,
    .client_secret = client_secret,
  };

  return _az_identity_credential_init(
      &(self->_internal.credential),
      (az_identity_credential_func)_az_identity_client_secret_credential_credential_func);
}
