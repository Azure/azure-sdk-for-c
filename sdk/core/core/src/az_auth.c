// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_auth.h>

#include <az_contract.h>
#include <az_http_client.h>
#include <az_http_request_builder.h>
#include <az_json_get.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_uri.h>

#include <assert.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_auth_init_client_credentials(
    az_auth_credentials * const out_result,
    az_span const tenant_id,
    az_span const client_id,
    az_span const client_secret) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_SPAN(tenant_id);
  AZ_CONTRACT_ARG_VALID_SPAN(client_id);
  AZ_CONTRACT_ARG_VALID_SPAN(client_secret);

  *out_result = (az_auth_credentials){
    .tenant_id = tenant_id,
    .kind = AZ_AUTH_KIND_CLIENT_CREDENTIALS,
    .data = { .client_credentials = { .client_id = client_id, .client_secret = client_secret } }
  };

  return AZ_OK;
}

enum {
  AZ_AUTH_GET_TOKEN_MIN_BUFFER
  = 1350, // if you measure the length of the login.microsoftonline.com's response, it is
          // around 1324 characters for key vault service.
  AZ_AUTH_URLENCODE_FACTOR = 3, // maximum characters needed when URL encoding (3x the original)
};

AZ_NODISCARD az_result az_auth_get_token(
    az_auth_credentials const credentials,
    az_span const resource_url,
    az_mut_span const response_buf,
    az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_SPAN(resource_url);

  AZ_CONTRACT_ARG_VALID_MUT_SPAN(response_buf);
  AZ_CONTRACT_ARG_VALID_SPAN(credentials.tenant_id);

  AZ_CONTRACT(credentials.kind == AZ_AUTH_KIND_CLIENT_CREDENTIALS, AZ_ERROR_ARG);
  AZ_CONTRACT_ARG_VALID_SPAN(credentials.data.client_credentials.client_id);
  AZ_CONTRACT_ARG_VALID_SPAN(credentials.data.client_credentials.client_secret);

  {
    AZ_CONTRACT(resource_url.size >= 12, AZ_ERROR_ARG);
    AZ_CONTRACT(credentials.tenant_id.size >= 32, AZ_ERROR_ARG);
    AZ_CONTRACT(credentials.data.client_credentials.client_id.size >= 32, AZ_ERROR_ARG);
    AZ_CONTRACT(credentials.data.client_credentials.client_secret.size >= 32, AZ_ERROR_ARG);
  }

  static az_span const auth_url1 = AZ_CONST_STR("https://login.microsoftonline.com/");
  static az_span const auth_url2 = AZ_CONST_STR("/oauth2/token");

  static az_span const auth_body1 = AZ_CONST_STR("grant_type=client_credentials&client_id=");
  static az_span const auth_body2 = AZ_CONST_STR("&client_secret=");
  static az_span const auth_body3 = AZ_CONST_STR("&resource=");

  size_t const auth_url_maxsize
      = auth_url1.size + (credentials.tenant_id.size * AZ_AUTH_URLENCODE_FACTOR) + auth_url2.size;

  AZ_CONTRACT(auth_url_maxsize <= (size_t) ~(uint16_t)0, AZ_ERROR_ARG);

  {
    AZ_CONTRACT(response_buf.size >= AZ_AUTH_GET_TOKEN_MIN_BUFFER, AZ_ERROR_BUFFER_OVERFLOW);

    size_t const request_elements[] = {
      credentials.tenant_id.size * AZ_AUTH_URLENCODE_FACTOR,
      credentials.data.client_credentials.client_id.size * AZ_AUTH_URLENCODE_FACTOR,
      credentials.data.client_credentials.client_secret.size * AZ_AUTH_URLENCODE_FACTOR,
      resource_url.size * AZ_AUTH_URLENCODE_FACTOR,
      auth_url_maxsize,
    };

    size_t required_request_size
        = auth_url1.size + auth_url2.size + auth_body1.size + auth_body2.size + auth_body3.size;

    for (size_t i = 0; i < (sizeof(request_elements) / sizeof(request_elements[0])); ++i) {
      required_request_size += request_elements[i];
      AZ_CONTRACT(required_request_size > request_elements[i], AZ_ERROR_BUFFER_OVERFLOW);
    }

    AZ_CONTRACT(response_buf.size >= required_request_size, AZ_ERROR_BUFFER_OVERFLOW);
  }

  {
    AZ_CONTRACT(!az_span_is_overlap(az_mut_span_to_span(response_buf), resource_url), AZ_ERROR_ARG);

    AZ_CONTRACT(
        !az_span_is_overlap(az_mut_span_to_span(response_buf), credentials.tenant_id),
        AZ_ERROR_ARG);

    AZ_CONTRACT(
        !az_span_is_overlap(
            az_mut_span_to_span(response_buf), credentials.data.client_credentials.client_id),
        AZ_ERROR_ARG);

    AZ_CONTRACT(
        !az_span_is_overlap(
            az_mut_span_to_span(response_buf), credentials.data.client_credentials.client_secret),
        AZ_ERROR_ARG);
  }

  az_span auth_url = { 0 };
  az_span auth_body = { 0 };
  {
    az_span_builder builder = az_span_builder_create(response_buf);

    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_url1));
    AZ_RETURN_IF_FAILED(az_uri_encode(credentials.tenant_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_url2));

    auth_url = az_span_builder_result(&builder);
    builder = az_span_builder_create(az_mut_span_take(response_buf, auth_url.size));

    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_body1));
    AZ_RETURN_IF_FAILED(az_uri_encode(credentials.data.client_credentials.client_id, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_body2));
    AZ_RETURN_IF_FAILED(az_uri_encode(credentials.data.client_credentials.client_secret, &builder));
    AZ_RETURN_IF_FAILED(az_span_builder_append(&builder, auth_body3));
    AZ_RETURN_IF_FAILED(az_uri_encode(resource_url, &builder));

    auth_body = az_span_builder_result(&builder);
  }

  {
    assert(auth_url.size <= auth_url_maxsize);

    az_http_request_builder hrb = { 0 };
    AZ_RETURN_IF_FAILED(az_http_request_builder_init(
        &hrb,
        (az_mut_span){ .begin = response_buf.begin + response_buf.size - auth_url.size,
                       .size = auth_url.size },
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
