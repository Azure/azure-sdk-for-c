// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_identity_client_secret_credential.h>

#include <az_http_pipeline.h>
#include <az_http_request_builder.h>
#include <_internal/az_http_response_parser.h>
#include <_internal/az_uri.h>
#include <_internal/az_url.h>
#include <az_contract.h>
#include <az_identity_access_token_context.h>
#include <az_identity_credential.h>
#include <az_json_get.h>
#include <az_span_builder.h>
#include <az_str.h>

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
  return p_policies[0].pfnc_process(&(p_policies[1]), p_policies[0].data, hrb, response);
}

AZ_INLINE AZ_NODISCARD az_result
_az_identity_client_secret_credential_ms_oauth2_send_get_token_request(
    az_identity_access_token_context const * const token_context,
    az_mut_span const auth_url_buf,
    az_mut_span const auth_body_buf,
    az_mut_span const hrb_buf,
    az_http_response * response,
    clock_t * const requested_at) {
  AZ_CONTRACT_ARG_NOT_NULL(token_context->_internal.credential);
  az_identity_client_secret_credential const * const credential
      = (az_identity_client_secret_credential const *)(token_context->_internal.credential);

  az_span auth_url = { 0 };
  {
    az_span_builder builder = az_span_builder_create(auth_url_buf);

    AZ_RETURN_IF_FAILED(
        az_span_builder_append(&builder, AZ_STR("https://login.microsoftonline.com/")));

    AZ_RETURN_IF_FAILED(az_uri_encode(credential->tenant_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("/oauth2/v2.0/token")));

    auth_url = az_span_builder_result(&builder);
  }

  az_span auth_body = { 0 };
  {
    az_span_builder builder = az_span_builder_create(auth_body_buf);

    AZ_RETURN_IF_FAILED(
        az_span_builder_append(&builder, AZ_STR("grant_type=client_credentials&client_id=")));

    AZ_RETURN_IF_FAILED(az_uri_encode(credential->client_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("&client_secret=")));
    AZ_RETURN_IF_FAILED(az_uri_encode(credential->client_secret, &builder));

    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("&scope=")));
    AZ_RETURN_IF_FAILED(az_uri_encode(token_context->_internal.scope, &builder));

    auth_body = az_span_builder_result(&builder);
  }

  az_http_request_builder hrb = { 0 };
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb, hrb_buf, (uint16_t)auth_url.size, AZ_HTTP_METHOD_VERB_POST, auth_url, auth_body));

  static az_http_pipeline pipeline = {
      .policies = {
        { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_retry, .data = NULL },
        { .pfnc_process = _az_http_pipeline_no_op, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_logging, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_transport, .data = NULL },
        { .pfnc_process = NULL, .data = NULL },
      },
    };

  *requested_at = clock();
  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, &hrb, response));

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_ms_oauth2_get_token(
    az_identity_access_token_context const * const token_context,
    az_http_response * const response) {
  clock_t requested_at = 0;
  {
    uint8_t auth_url_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_URL_BUF_SIZE] = { 0 };
    uint8_t auth_body_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_AUTH_BODY_BUF_SIZE] = { 0 };
    uint8_t hrb_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_HRB_BUF_SIZE] = { 0 };

    az_mut_span const auth_url = AZ_SPAN_FROM_ARRAY(auth_url_buf);
    az_mut_span const auth_body = AZ_SPAN_FROM_ARRAY(auth_body_buf);
    az_mut_span const hrb_buf_span = AZ_SPAN_FROM_ARRAY(hrb_buf);

    az_result const token_request_result
        = _az_identity_client_secret_credential_ms_oauth2_send_get_token_request(
            token_context, auth_url, auth_body, hrb_buf_span, response, &requested_at);

    az_mut_span_fill(hrb_buf_span, '#');
    az_mut_span_fill(auth_body, '#');
    az_mut_span_fill(auth_url, '#');
    AZ_RETURN_IF_FAILED(token_request_result);
  }

  az_span body = { 0 };
  {
    az_http_response_parser parser = { 0 };
    AZ_RETURN_IF_FAILED(
        az_http_response_parser_init(&parser, az_span_builder_result(&response->builder)));

    az_http_response_status_line status_line = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&parser, &status_line));
    if (status_line.status_code != AZ_HTTP_STATUS_CODE_OK) {
      return AZ_ERROR_HTTP_PAL;
    }

    AZ_RETURN_IF_FAILED(az_http_response_parser_skip_headers(&parser));
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_body(&parser, &body));
  }

  {
    az_json_token value;

    clock_t expiration_clock = 0;
    if (requested_at > 0) {
      double expiration_seconds = 0;
      if (az_succeeded(az_json_get_object_member(body, AZ_STR("expires_in"), &value))
          && az_succeeded(az_json_token_get_number(value, &expiration_seconds))) {
        if (expiration_seconds > 0) {
          expiration_clock = (((clock_t)expiration_seconds) - (3 * 60)) * CLOCKS_PER_SEC;
        }
      }
    }

    az_span token_str = { 0 };
    AZ_RETURN_IF_FAILED(az_json_get_object_member(body, AZ_STR("access_token"), &value));
    AZ_RETURN_IF_FAILED(az_json_token_get_string(value, &token_str));

    az_mut_span const token_buf
        = AZ_SPAN_FROM_ARRAY(token_context->_internal.token->_internal.token_buf);

    az_span_builder builder = az_span_builder_create(token_buf);
    az_mut_span_fill(token_buf, 0);

    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("Bearer ")));
    az_result const token_append_result = az_span_builder_append(&builder, token_str);

    if (az_succeeded(token_append_result)) {
      token_context->_internal.token->_internal.token_size = az_span_builder_result(&builder).size;

      clock_t const expiration = requested_at + expiration_clock;
      if (expiration > 0) {
        token_context->_internal.token->_internal.token_expiration = expiration;
      }
    } else {
      az_mut_span_fill(token_buf, 'X');
      return token_append_result;
    }
  }

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_identity_client_secret_credential_renew_token_credential(
    az_identity_access_token_context const * const token_context) {
  uint8_t response_buf[_az_IDENTITY_CLIENT_SECRET_CREDENTIAL_RESPONSE_BUF_SIZE] = { 0 };

  az_http_response http_response = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_init(
      &http_response, az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(response_buf))));

  az_result const result
      = _az_identity_client_secret_credential_ms_oauth2_get_token(token_context, &http_response);

  az_mut_span_fill(http_response.builder.buffer, '#');
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
    az_http_request_builder * const hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(token_context);
  AZ_CONTRACT_ARG_NOT_NULL(token_context->_internal.token);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);

  {
    az_url url = { 0 };

    AZ_RETURN_IF_FAILED(
        az_succeeded(az_url_parse(az_span_builder_result(&hrb->url_builder), &url)));

    if (!az_span_is_equal_ignoring_case(url.scheme, AZ_STR("https"))) {
      return AZ_ERROR_ARG;
    }
  }

  AZ_RETURN_IF_FAILED(_az_identity_client_secret_credential_ensure_token_credential(token_context));

  return az_http_request_builder_append_header(
      hrb,
      AZ_STR("authorization"),
      (az_span){
          .begin = token_context->_internal.token->_internal.token_buf,
          .size = token_context->_internal.token->_internal.token_size,
      });
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
