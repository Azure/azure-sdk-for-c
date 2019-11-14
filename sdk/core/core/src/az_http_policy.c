// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_auth.h>
#include <az_http_client.h>
#include <az_http_pipeline.h>
#include <az_http_policy.h>
#include <az_http_request_builder.h>
#include <az_mut_span.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_url.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_http_pipeline_nextpolicy(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (p_policies[0].pfnc_process == NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies[0].pfnc_process(&(p_policies[1]), p_policies[0].data, hrb, response);
}

static az_span const AZ_MS_CLIENT_REQUESTID = AZ_CONST_STR("x-ms-client-request-id");

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  (void)data;
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  // TODO - add a UUID create implementation
  az_span const uniqueid = AZ_CONST_STR("123e4567-e89b-12d3-a456-426655440000");

  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(hrb, AZ_MS_CLIENT_REQUESTID, uniqueid));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  (void)data;
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // Retry logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

enum { AZ_HTTP_POLICY_AUTH_BUFFER_SIZE = 2 * 1024 };

// Being given
// "https://NNNNNNNN.vault.azure.net/secrets/Password/XXXXXXXXXXXXXXXXXXXX?api-version=7.0", gives
// back "https://vault.azure.net" (needed for authentication).
static AZ_NODISCARD az_result
az_auth_get_resource_url(az_span const request_url, az_span_builder * p_builder) {
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

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  az_auth_credentials const * const credentials = (az_auth_credentials const *)(data);
  if (credentials == NULL || credentials->kind == AZ_AUTH_KIND_NONE) {
    return az_http_pipeline_nextpolicy(p_policies, hrb, response);
  }

  uint8_t buf[AZ_HTTP_POLICY_AUTH_BUFFER_SIZE] = { 0 };
  az_mut_span entire_buf = AZ_SPAN_FROM_ARRAY(buf);
  az_mut_span post_bearer = { 0 };
  az_mut_span bearer = { 0 };
  AZ_RETURN_IF_FAILED(az_mut_span_copy(entire_buf, AZ_STR("Bearer "), &bearer));
  post_bearer = az_mut_span_drop(entire_buf, bearer.size);

  az_span_builder auth_url_builder = az_span_builder_create(post_bearer);
  AZ_RETURN_IF_FAILED(az_auth_get_resource_url(az_mut_span_to_span(hrb->url), &auth_url_builder));
  az_span const auth_url = az_span_builder_result(&auth_url_builder);

  az_span token = { 0 };
  AZ_RETURN_IF_FAILED(az_auth_get_token(
      *credentials, auth_url, az_mut_span_drop(post_bearer, auth_url.size), &token));

  az_mut_span unused;
  AZ_RETURN_IF_FAILED(az_mut_span_move(post_bearer, token, &unused));

  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(
      hrb,
      AZ_STR("authorization"),
      (az_span){ .begin = bearer.begin, .size = bearer.size + token.size }));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  (void)data;
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  (void)data;
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  (void)data;
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  (void)data;
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  // Transport policy is the last policy
  //  If a policy exists after the transport policy
  if (p_policies[0].pfnc_process != NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return az_http_client_send_request(hrb, response);
}
