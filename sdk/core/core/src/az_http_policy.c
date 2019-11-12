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

  return p_policies[0].pfnc_process(&(p_policies[1]), hrb, response);
}

static az_span const AZ_MS_CLIENT_REQUESTID = AZ_CONST_STR("x-ms-client-request-id");

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
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
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // Retry logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

enum { AZ_HTTP_POLICY_AUTH_BUFFER_SIZE = 2 * 1024 };

static AZ_NODISCARD az_result
az_auth_get_resource_url(az_span const request_url, az_span_builder * p_builder) {
  size_t scheme_size = 0;

  uint8_t const * url = request_url.begin;
  size_t const url_size = request_url.size;
  for (size_t i = 0; i < url_size; ++i) {
    if (url[i] == ':' && (i < url_size + 2) && url[i + 1] == '/' && url[i + 2] == '/') {
      scheme_size = i + 3;
      break;
    }
  }

  if (scheme_size == 0) {
    return AZ_ERROR_ARG;
  }

  size_t host_size = url_size - scheme_size;
  for (size_t i = scheme_size; i < url_size; ++i) {
    if (url[i] == '/') {
      host_size = i - scheme_size;
      break;
    }
  }

  if (host_size == 0) {
    return AZ_ERROR_ARG;
  }

  size_t lvl3domain_size = 0;
  {
    uint8_t dot = 0;
    for (size_t i = scheme_size + host_size; i >= scheme_size; --i) {
      if (url[i] == '.') {
        ++dot;
      }

      if (dot == 3) {
        lvl3domain_size = host_size - ((i + 1) - scheme_size);
        break;
      }
    }

    if (dot == 2) {
      lvl3domain_size = host_size;
    }
  }

  if (lvl3domain_size == 0) {
    return AZ_ERROR_ARG;
  }

  AZ_RETURN_IF_FAILED(
      az_span_builder_append(p_builder, (az_span){ .begin = url, .size = scheme_size }));

  AZ_RETURN_IF_FAILED(az_span_builder_append(
      p_builder,
      (az_span){ .begin = url + scheme_size + host_size - lvl3domain_size,
                 .size = lvl3domain_size }));

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  if (p_policies->data.credentials.kind == AZ_AUTH_KIND_NONE) {
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
      p_policies->data.credentials,
      auth_url,
      az_mut_span_drop(post_bearer, auth_url.size),
      &token));

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
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {

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
