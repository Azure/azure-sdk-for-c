// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_auth.h>

#include <az_contract.h>
#include <az_http_client.h>
#include <az_http_request_builder.h>
#include <az_json_get.h>
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
  MIN_BUFFER = 1350, // if you measure the length of the login.microsoftonline.com's response, it is
                     // around 1324 characters for key vault service.
  URLENCODE_FACTOR = 3, // maximum characters needed when URL encoding (3x the original)
  NREQUEST_ELEMENTS = 5
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
      = auth_url1.size + (credentials.tenant_id.size * URLENCODE_FACTOR) + auth_url2.size;

  AZ_CONTRACT(auth_url_maxsize <= (size_t) ~(uint16_t)0, AZ_ERROR_ARG);

  {
    AZ_CONTRACT(response_buf.size >= MIN_BUFFER, AZ_ERROR_BUFFER_OVERFLOW);

    size_t const request_elements[NREQUEST_ELEMENTS]
        = { credentials.tenant_id.size * URLENCODE_FACTOR,
            credentials.data.client_credentials.client_id.size * URLENCODE_FACTOR,
            credentials.data.client_credentials.client_secret.size * URLENCODE_FACTOR,
            resource_url.size * URLENCODE_FACTOR,
            auth_url_maxsize };

    size_t required_request_size
        = auth_url1.size + auth_url2.size + auth_body1.size + auth_body2.size + auth_body3.size;

    for (size_t i = 0; i < NREQUEST_ELEMENTS; ++i) {
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

  az_span auth_url = { 0, 0 };
  az_span auth_body = { 0, 0 };
  {
    az_mut_span buf = response_buf;
    az_mut_span tmp_span = { 0, 0 };

    {
      auth_url.begin = buf.begin;

      AZ_RETURN_IF_FAILED(az_mut_span_copy(buf, auth_url1, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(az_uri_encode(buf, credentials.tenant_id, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(az_mut_span_copy(buf, auth_url2, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      auth_url.size = (buf.begin - auth_url.begin);
    }

    {
      auth_body.begin = buf.begin;

      AZ_RETURN_IF_FAILED(az_mut_span_copy(buf, auth_body1, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(
          az_uri_encode(buf, credentials.data.client_credentials.client_id, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(az_mut_span_copy(buf, auth_body2, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(
          az_uri_encode(buf, credentials.data.client_credentials.client_secret, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(az_mut_span_copy(buf, auth_body3, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      AZ_RETURN_IF_FAILED(az_uri_encode(buf, resource_url, &tmp_span));
      buf = (az_mut_span){
        .begin = buf.begin + tmp_span.size,
        .size = buf.size - tmp_span.size,
      };

      auth_body.size = (buf.begin - auth_body.begin);
    }
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

  AZ_RETURN_IF_FAILED(az_json_get_object_member_string(
      az_mut_span_to_span(response_buf), AZ_STR("access_token"), out_result));

  return AZ_OK;
}
