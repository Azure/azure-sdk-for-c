// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_client_secret_credential.h>

#include <az_contract.h>
#include <az_http_client.h>
#include <az_http_request_builder.h>
#include <az_json_get.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_uri.h>
#include <az_url.h>

#include <assert.h>

#include <_az_cfg.h>

enum {
  AZ_TOKEN_CREDENTIAL_GET_TOKEN_MIN_BUFFER
  = 1350, // if you measure the length of the login.microsoftonline.com's response, it is
          // around 1324 characters for key vault service.
  AZ_TOKEN_CREDENTIAL_GET_TOKEN_URLENCODE_FACTOR
  = 3, // maximum characters needed when URL encoding (3x the original)
};

static AZ_NODISCARD az_result az_token_credential_get_token(
    az_client_secret_credential const * const credential,
    az_span const resource_url,
    az_mut_span const response_buf,
    az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(credential);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_SPAN(resource_url);

  AZ_CONTRACT_ARG_VALID_MUT_SPAN(response_buf);
  AZ_CONTRACT_ARG_VALID_SPAN(credential->tenant_id);

  AZ_CONTRACT_ARG_VALID_SPAN(credential->client_id);
  AZ_CONTRACT_ARG_VALID_SPAN(credential->client_secret);

  {
    AZ_CONTRACT(resource_url.size >= 12, AZ_ERROR_ARG);
    AZ_CONTRACT(credential->tenant_id.size >= 32, AZ_ERROR_ARG);
    AZ_CONTRACT(credential->client_id.size >= 32, AZ_ERROR_ARG);
    AZ_CONTRACT(credential->client_secret.size >= 32, AZ_ERROR_ARG);
  }

  static az_span const auth_url1 = AZ_CONST_STR("https://login.microsoftonline.com/");
  static az_span const auth_url2 = AZ_CONST_STR("/oauth2/token");

  static az_span const auth_body1 = AZ_CONST_STR("grant_type=client_credentials&client_id=");
  static az_span const auth_body2 = AZ_CONST_STR("&client_secret=");
  static az_span const auth_body3 = AZ_CONST_STR("&resource=");

  size_t const auth_url_maxsize = auth_url1.size
      + (credential->tenant_id.size * AZ_TOKEN_CREDENTIAL_GET_TOKEN_URLENCODE_FACTOR)
      + auth_url2.size;

  AZ_CONTRACT(auth_url_maxsize <= (size_t) ~(uint16_t)0, AZ_ERROR_ARG);

  {
    AZ_CONTRACT(
        response_buf.size >= AZ_TOKEN_CREDENTIAL_GET_TOKEN_MIN_BUFFER, AZ_ERROR_BUFFER_OVERFLOW);

    size_t const request_elements[] = {
      credential->tenant_id.size * AZ_TOKEN_CREDENTIAL_GET_TOKEN_URLENCODE_FACTOR,
      credential->client_id.size * AZ_TOKEN_CREDENTIAL_GET_TOKEN_URLENCODE_FACTOR,
      credential->client_secret.size * AZ_TOKEN_CREDENTIAL_GET_TOKEN_URLENCODE_FACTOR,
      resource_url.size * AZ_TOKEN_CREDENTIAL_GET_TOKEN_URLENCODE_FACTOR,
      auth_url_maxsize,
    };

    size_t required_request_size
        = auth_url1.size + auth_url2.size + auth_body1.size + auth_body2.size + auth_body3.size;

    for (size_t i = 0; i < AZ_ARRAY_SIZE(request_elements); ++i) {
      required_request_size += request_elements[i];
      AZ_CONTRACT(required_request_size > request_elements[i], AZ_ERROR_BUFFER_OVERFLOW);
    }

    AZ_CONTRACT(response_buf.size >= required_request_size, AZ_ERROR_BUFFER_OVERFLOW);
  }

  {
    AZ_CONTRACT(!az_span_is_overlap(az_mut_span_to_span(response_buf), resource_url), AZ_ERROR_ARG);

    AZ_CONTRACT(
        !az_span_is_overlap(az_mut_span_to_span(response_buf), credential->tenant_id),
        AZ_ERROR_ARG);

    AZ_CONTRACT(
        !az_span_is_overlap(az_mut_span_to_span(response_buf), credential->client_id),
        AZ_ERROR_ARG);

    AZ_CONTRACT(
        !az_span_is_overlap(az_mut_span_to_span(response_buf), credential->client_secret),
        AZ_ERROR_ARG);
  }

  az_span auth_url = { 0 };
  az_span auth_body = { 0 };
  {
    az_span_builder builder = az_span_builder_create(response_buf);

    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_url1));
    AZ_RETURN_IF_FAILED(az_uri_encode(credential->tenant_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_url2));

    auth_url = az_span_builder_result(&builder);
    builder = az_span_builder_create(az_mut_span_drop(response_buf, auth_url.size));

    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_body1));
    AZ_RETURN_IF_FAILED(az_uri_encode(credential->client_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_body2));
    AZ_RETURN_IF_FAILED(az_uri_encode(credential->client_secret, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_body3));
    AZ_RETURN_IF_FAILED(az_uri_encode(resource_url, &builder));

    auth_body = az_span_builder_result(&builder);
  }

  {
    assert(auth_url.size <= auth_url_maxsize);

    az_http_request_builder hrb = { 0 };
    AZ_RETURN_IF_FAILED(az_http_request_builder_init(
        &hrb,
        (az_mut_span){
            .begin = response_buf.begin + response_buf.size - auth_url.size,
            .size = auth_url.size,
        },
        (uint16_t)auth_url.size,
        AZ_HTTP_METHOD_VERB_POST,
        auth_url));

    AZ_RETURN_IF_FAILED(az_http_request_builder_add_body(&hrb, auth_body));
    AZ_RETURN_IF_FAILED(az_http_client_send_request_and_get_body(&hrb, &response_buf));
  }

  {
    az_json_value value;
    AZ_RETURN_IF_FAILED(az_json_get_object_member(
        az_mut_span_to_span(response_buf), AZ_STR("access_token"), &value));
    AZ_RETURN_IF_FAILED(az_json_value_get_string(&value, out_result));
  }

  return AZ_OK;
}

// Being given
// "https://NNNNNNNN.vault.azure.net/secrets/Password/XXXXXXXXXXXXXXXXXXXX?api-version=7.0", gives
// back "https://vault.azure.net" (needed for authentication).
static AZ_NODISCARD az_result
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

static AZ_NODISCARD az_result az_token_credential_add_token_header(
    az_client_secret_credential * const credential,
    az_http_request_builder * const hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(credential);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);

  az_mut_span entire_buf = (az_mut_span){
    .begin = credential->token_credential.token,
    .size = sizeof(credential->token_credential.token),
  };
  az_mut_span post_bearer = { 0 };
  az_mut_span bearer = { 0 };
  AZ_RETURN_IF_FAILED(az_mut_span_copy(entire_buf, AZ_STR("Bearer "), &bearer));
  post_bearer = az_mut_span_drop(entire_buf, bearer.size);

  az_span_builder auth_url_builder = az_span_builder_create(post_bearer);
  AZ_RETURN_IF_FAILED(
      az_token_credential_get_resource_url(az_mut_span_to_span(hrb->url), &auth_url_builder));

  az_span const auth_url = az_span_builder_result(&auth_url_builder);

  az_span token = { 0 };
  AZ_RETURN_IF_FAILED(az_token_credential_get_token(
      credential, auth_url, az_mut_span_drop(post_bearer, auth_url.size), &token));

  az_mut_span unused;
  AZ_RETURN_IF_FAILED(az_mut_span_move(post_bearer, token, &unused));

  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      hrb,
      AZ_STR("authorization"),
      (az_span){ .begin = bearer.begin, .size = bearer.size + token.size }));

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
