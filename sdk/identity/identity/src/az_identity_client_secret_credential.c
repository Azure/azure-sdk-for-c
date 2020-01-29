// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_identity_client_secret_credential.h>

#include <az_clock_internal.h>
#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_identity_access_token_context.h>
#include <az_identity_credential.h>
#include <az_json.h>
#include <az_span_internal.h>

#include <az_uri_internal.h>
#include <az_url_internal.h>

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

static AZ_NODISCARD az_result _az_http_pipeline_no_op(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;
  return p_policies[0].process(&(p_policies[1]), p_policies[0].data, hrb, response);
}

AZ_INLINE AZ_NODISCARD az_result
_az_identity_client_secret_credential_ms_oauth2_send_get_token_request(
    az_identity_access_token_context const * const token_context,
    az_span auth_url_buf,
    az_span auth_body_buf,
    az_span hrb_buf,
    az_http_response * response,
    uint64_t * const requested_at_msec) {
  AZ_CONTRACT_ARG_NOT_NULL(token_context->_internal.credential);
  az_identity_client_secret_credential const * const credential
      = (az_identity_client_secret_credential const *)(token_context->_internal.credential);

  {
    AZ_RETURN_IF_FAILED(az_span_append(
        auth_url_buf, AZ_SPAN_FROM_STR("https://login.microsoftonline.com/"), &auth_url_buf));

    AZ_RETURN_IF_FAILED(az_uri_encode(credential->tenant_id, &auth_url_buf));
    AZ_RETURN_IF_FAILED(
        az_span_append(auth_url_buf, AZ_SPAN_FROM_STR("/oauth2/v2.0/token"), &auth_url_buf));
  }
  {
    AZ_RETURN_IF_FAILED(az_span_append(
        auth_body_buf,
        AZ_SPAN_FROM_STR("grant_type=client_credentials&client_id="),
        &auth_body_buf));

    AZ_RETURN_IF_FAILED(az_uri_encode(credential->client_id, &auth_body_buf));
    AZ_RETURN_IF_FAILED(
        az_span_append(auth_body_buf, AZ_SPAN_FROM_STR("&client_secret="), &auth_body_buf));
    AZ_RETURN_IF_FAILED(az_uri_encode(credential->client_secret, &auth_body_buf));

    AZ_RETURN_IF_FAILED(az_span_append(auth_body_buf, AZ_SPAN_FROM_STR("&scope="), &auth_body_buf));
    AZ_RETURN_IF_FAILED(az_uri_encode(token_context->_internal.scope, &auth_body_buf));
  }

  az_http_request_builder hrb = { 0 };
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      hrb_buf,
      az_span_length(auth_url_buf),
      AZ_HTTP_METHOD_VERB_POST,
      auth_url_buf,
      auth_body_buf));

  static az_http_pipeline pipeline = {
      .policies = {
        { .process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
        { .process = az_http_pipeline_policy_retry, .data = NULL },
        { .process = _az_http_pipeline_no_op, .data = NULL },
        { .process = az_http_pipeline_policy_logging, .data = NULL },
        { .process = az_http_pipeline_policy_bufferresponse, .data = NULL },
        { .process = az_http_pipeline_policy_distributedtracing, .data = NULL },
        { .process = az_http_pipeline_policy_transport, .data = NULL },
        { .process = NULL, .data = NULL },
      },
    };

  *requested_at_msec = _az_clock_msec();
  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, &hrb, response));

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_ms_oauth2_get_token(
    az_identity_access_token_context const * const token_context,
    az_http_response * const response) {
  uint64_t requested_at_msec = 0;
  {
    uint8_t auth_url_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_URL_BUF_SIZE] = { 0 };
    uint8_t auth_body_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_BODY_BUF_SIZE] = { 0 };
    uint8_t hrb_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_HRB_BUF_SIZE] = { 0 };

    az_span const auth_url = AZ_SPAN_FROM_BUFFER(auth_url_buf);
    az_span const auth_body = AZ_SPAN_FROM_BUFFER(auth_body_buf);
    az_span const hrb_buf_span = AZ_SPAN_FROM_BUFFER(hrb_buf);

    az_result const token_request_result
        = _az_identity_client_secret_credential_ms_oauth2_send_get_token_request(
            token_context, auth_url, auth_body, hrb_buf_span, response, &requested_at_msec);

    az_span_fill(hrb_buf_span, '#');
    az_span_fill(auth_body, '#');
    az_span_fill(auth_url, '#');
    AZ_RETURN_IF_FAILED(token_request_result);
  }

  az_span body = { 0 };
  {
    az_http_response_parser parser = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_parser_init(&parser, response->builder));

    az_http_response_status_line status_line = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&parser, &status_line));
    if (status_line.status_code != AZ_HTTP_STATUS_CODE_OK) {
      return AZ_ERROR_HTTP_PAL;
    }

    AZ_RETURN_IF_FAILED(az_http_response_parser_skip_headers(&parser));
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_body(&parser, &body));
  }

  {
    az_json_token token;

    uint64_t expires_in_msec = 0;
    if (requested_at_msec > 0) {
      double expires_in_seconds = 0;
      if (az_succeeded(az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/expires_in"), &token))
          && az_succeeded(az_json_token_get_number(token, &expires_in_seconds))) {
          && expires_in_seconds > 0) {
        double const norefresh_period_msec = (expires_in_seconds - (double)(3 * 60)) * 1000;
        if (((int64_t)norefresh_period_msec) > 0) {
          expires_in_msec = (uint64_t)norefresh_period_msec;
        }
      }
    }

    az_span token_str = { 0 };
    AZ_RETURN_IF_FAILED(az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/access_token"), &token));
    AZ_RETURN_IF_FAILED(az_json_token_get_string(token, &token_str));

    az_span token_buf = AZ_SPAN_FROM_BUFFER(token_context->_internal.token->_internal.token_buf);

    az_span_fill(token_buf, 0);

    AZ_RETURN_IF_FAILED(az_span_append(token_buf, AZ_SPAN_FROM_STR("Bearer "), &token_buf));
    az_result const token_append_result = az_span_append(token_buf, token_str, &token_buf);

    if (az_succeeded(token_append_result)) {
      token_context->_internal.token->_internal.token_size = az_span_length(token_buf);

      uint64_t const refresh_after_msec = requested_at_msec + expires_in_msec;
      if (refresh_after_msec > 0) {
        token_context->_internal.token->_internal.token_refresh_after_msec = refresh_after_msec;
      }
    } else {
      az_span_fill(token_buf, 'X');
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

  az_span_fill(http_response.builder, '#');
  return result;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_ensure_token_credential(
    az_identity_access_token_context const * const token_context) {
  uint64_t const token_refresh_after_msec
      = token_context->_internal.token->_internal.token_refresh_after_msec;

  if (token_refresh_after_msec > 0) {
    uint64_t const current_msec = _az_clock_msec();
    if (current_msec > 0 && current_msec < token_refresh_after_msec) {
      return AZ_OK;
    }
  }

  return _az_identity_client_secret_credential_renew_token_credential(token_context);
}

static AZ_NODISCARD az_result _az_identity_client_secret_credential_credential_func(
    az_identity_access_token_context const * const token_context,
    az_http_request_builder * const hrb) {
  // TODO: thread safety
  AZ_CONTRACT_ARG_NOT_NULL(token_context);
  AZ_CONTRACT_ARG_NOT_NULL(token_context->_internal.token);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);

  {
    az_url url = { 0 };

    AZ_RETURN_IF_FAILED(az_succeeded(az_url_parse(hrb->url_builder, &url)));

    if (!az_span_is_content_equal_ignoring_case(url.scheme, AZ_SPAN_FROM_STR("https"))) {
      return AZ_ERROR_ARG;
    }
  }

  AZ_RETURN_IF_FAILED(_az_identity_client_secret_credential_ensure_token_credential(token_context));
  int32_t token_length = token_context->_internal.token->_internal.token_size;
  return az_http_request_builder_append_header(
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
