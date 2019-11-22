// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_client_secret_credential.h>

#include <az_contract.h>
#include <az_http_pipeline.h>
#include <az_http_request_builder.h>
#include <az_http_response_parser.h>
#include <az_json_get.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_uri.h>
#include <az_url.h>

#include <assert.h>

#include <_az_cfg.h>

enum {
  AZ_TOKEN_CREDENTIAL_RESPONSE_BUF_SIZE = 5 * (1024 / 2),
  AZ_TOKEN_CREDENTIAL_AUTH_URL_BUF_SIZE = 100,
  AZ_TOKEN_CREDENTIAL_AUTH_BODY_BUF_SIZE = 200,
  AZ_TOKEN_CREDENTIAL_AUTH_RESOURCE_URL_BUF_SIZE = 100,
};

static AZ_NODISCARD az_result no_op_policy(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response const * const response) {
  (void)data;
  return p_policies[0].pfnc_process(&(p_policies[1]), p_policies[0].data, hrb, response);
}

AZ_INLINE AZ_NODISCARD az_result az_token_credential_send_get_token_request(
    az_client_secret_credential const * const credential,
    az_span const resource_url,
    az_mut_span const auth_url_buf,
    az_mut_span const auth_body_buf,
    az_mut_span const response_buf) {
  az_span auth_url = { 0 };
  {
    az_span_builder builder = az_span_builder_create(auth_url_buf);

    AZ_RETURN_IF_FAILED(
        az_span_builder_append(&builder, AZ_STR("https://login.microsoftonline.com/")));

    AZ_RETURN_IF_FAILED(az_uri_encode(credential->tenant_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("/oauth2/token")));

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
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("&resource=")));
    AZ_RETURN_IF_FAILED(az_uri_encode(resource_url, &builder));

    auth_body = az_span_builder_result(&builder);
  }

  az_http_request_builder hrb = { 0 };
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb, response_buf, (uint16_t)auth_url.size, AZ_HTTP_METHOD_VERB_POST, auth_url));

  AZ_RETURN_IF_FAILED(az_http_request_builder_add_body(&hrb, auth_body));

  static az_http_pipeline pipeline = {
      .policies = {
        { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_retry, .data = NULL },
        { .pfnc_process = no_op_policy, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_logging, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = NULL },
        { .pfnc_process = az_http_pipeline_policy_transport, .data = NULL },
        { .pfnc_process = NULL, .data = NULL },
      },
    };

  az_http_response response = { .value = response_buf };
  AZ_RETURN_IF_FAILED(az_http_pipeline_process(&pipeline, &hrb, &response));

  return AZ_OK;
}

// Being given
// "https://NNNNNNNN.vault.azure.net/secrets/Password/XXXXXXXXXXXXXXXXXXXX?api-version=7.0", gives
// back "https://vault.azure.net" (needed for authentication).
AZ_INLINE AZ_NODISCARD az_result
az_token_credential_get_resource_url(az_span const request_url, az_span_builder * p_builder) {
  az_url url = { 0 };
  if (!az_succeeded(az_url_parse(request_url, &url))) {
    return AZ_ERROR_ARG;
  }

  az_span domains[3] = { 0 };
  size_t const ndomains = AZ_ARRAY_SIZE(domains);
  for (size_t i = 0; i < ndomains; ++i) {
    if (!az_succeeded(az_host_read_domain(&url.authority.host, &domains[(ndomains - 1) - i]))) {
      return AZ_ERROR_ARG;
    }
  }

  // Add "https://"
  AZ_RETURN_IF_FAILED(az_span_builder_append(p_builder, url.scheme));
  AZ_RETURN_IF_FAILED(az_span_builder_append(p_builder, AZ_STR("://")));

  for (size_t i = 0; i < (ndomains - 1); ++i) { // This loop would add "vault.azure."
    AZ_RETURN_IF_FAILED(az_span_builder_append(p_builder, domains[i]));
    AZ_RETURN_IF_FAILED(az_span_builder_append(p_builder, AZ_STR(".")));
  }

  // We have to do this out of the loop so that we won't append an extra "." at the end.
  // So this expression is going to add the final "net" to an existing "https://vault.azure."
  AZ_RETURN_IF_FAILED(az_span_builder_append(p_builder, domains[ndomains - 1]));

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result az_token_credential_update(
    az_client_secret_credential * const credential,
    az_span const request_url,
    az_mut_span const response_buf) {
  {
    uint8_t auth_resource_url_buf[AZ_TOKEN_CREDENTIAL_AUTH_RESOURCE_URL_BUF_SIZE] = { 0 };
    az_mut_span const auth_resource_url = AZ_SPAN_FROM_ARRAY(auth_resource_url_buf);

    az_span resource_url = { 0 };
    {
      az_span_builder auth_resource_url_builder = az_span_builder_create(auth_resource_url);
      az_result const get_resource_url_result
          = az_token_credential_get_resource_url(request_url, &auth_resource_url_builder);
      if (az_succeeded(get_resource_url_result)) {
        resource_url = az_span_builder_result(&auth_resource_url_builder);
      } else {
        az_mut_span_memset(auth_resource_url, '#');
        return get_resource_url_result;
      }
    }

    uint8_t auth_url_buf[AZ_TOKEN_CREDENTIAL_AUTH_URL_BUF_SIZE] = { 0 };
    uint8_t auth_body_buf[AZ_TOKEN_CREDENTIAL_AUTH_BODY_BUF_SIZE] = { 0 };

    az_mut_span const auth_url = AZ_SPAN_FROM_ARRAY(auth_url_buf);
    az_mut_span const auth_body = AZ_SPAN_FROM_ARRAY(auth_body_buf);

    az_result const token_request_result = az_token_credential_send_get_token_request(
        credential, resource_url, auth_url, auth_body, response_buf);

    az_mut_span_memset(auth_body, '#');
    az_mut_span_memset(auth_url, '#');
    az_mut_span_memset(auth_resource_url, '#');
    AZ_RETURN_IF_FAILED(token_request_result);
  }
  az_span body = { 0 };
  {
    az_http_response_parser parser = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_parser_init(&parser, az_mut_span_to_span(response_buf)));

    az_http_response_status_line status_line = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&parser, &status_line));
    if (status_line.status_code != AZ_HTTP_STATUS_CODE_OK) {
      return AZ_ERROR_HTTP_PAL;
    }

    AZ_RETURN_IF_FAILED(az_http_response_parser_skip_headers(&parser));
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_body(&parser, &body));
  }

  {
    az_json_value value;
    AZ_RETURN_IF_FAILED(az_json_get_object_member(body, AZ_STR("access_token"), &value));

    az_span token = { 0 };
    AZ_RETURN_IF_FAILED(az_json_value_get_string(&value, &token));

    az_mut_span const token_buf = AZ_SPAN_FROM_ARRAY(credential->token_credential.token_buf);
    credential->token_credential.token = (az_mut_span){ 0 };
    az_mut_span_memset(token_buf, '\0');

    az_span_builder builder = az_span_builder_create(token_buf);
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, AZ_STR("Bearer ")));
    az_result const token_append_result = az_span_builder_append(&builder, token);

    if (az_succeeded(token_append_result)) {
      credential->token_credential.token = az_span_builder_mut_result(&builder);
    } else {
      az_mut_span_memset(token_buf, '#');
      return token_append_result;
    }
  }

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result az_token_credential_get_token(
    az_client_secret_credential * const credential,
    az_span const request_url) {
  uint8_t response_buf[AZ_TOKEN_CREDENTIAL_RESPONSE_BUF_SIZE] = { 0 };
  az_mut_span const response = AZ_SPAN_FROM_ARRAY(response_buf);
  
  az_result const credential_update_result
      = az_token_credential_update(credential, request_url, response);

  az_mut_span_memset(response, '#');
  return credential_update_result;
}

static AZ_NODISCARD az_result az_token_credential_add_token_header(
    az_client_secret_credential * const credential,
    az_http_request_builder * const hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(credential);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);

  AZ_RETURN_IF_FAILED(az_token_credential_get_token(credential, az_mut_span_to_span(hrb->url)));
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      hrb, AZ_STR("authorization"), az_mut_span_to_span(credential->token_credential.token)));

  return AZ_OK;
}

AZ_NODISCARD az_result az_client_secret_credential_init(
    az_client_secret_credential * const self,
    az_span const tenant_id,
    az_span const client_id,
    az_span const client_secret) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_VALID_SPAN(tenant_id);
  AZ_CONTRACT_ARG_VALID_SPAN(client_id);
  AZ_CONTRACT_ARG_VALID_SPAN(client_secret);

  *self = (az_client_secret_credential){
    .token_credential = { 0 },
    .tenant_id = tenant_id,
    .client_id = client_id,
    .client_secret = client_secret,
  };

  AZ_RETURN_IF_FAILED(az_token_credential_init(
      &(self->token_credential), (az_credential_func)az_token_credential_add_token_header));

  return AZ_OK;
}
