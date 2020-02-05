// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_log_private.h"
#include <az_clock_internal.h>
#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_pipeline_internal.h>
#include <az_log.h>
#include <az_log_internal.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_http_pipeline_nextpolicy(
    az_http_policy * p_policies,
    az_http_request * p_request,
    az_http_response * p_response) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(p_response);

  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (p_policies[0].process == NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies[0].process(&(p_policies[1]), p_policies[0].p_options, p_request, p_response);
}

static az_span AZ_MS_CLIENT_REQUESTID = AZ_SPAN_LITERAL_FROM_STR("x-ms-client-request-id");

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request_builder * p_request,
    az_http_response * p_response) {

  _az_http_policy_apiversion_options * options = (_az_http_policy_apiversion_options *)(p_options);

  if (options->add_as_header) {
    // Add the version as a header
    AZ_RETURN_IF_FAILED(
        az_http_request_builder_append_header(p_request, options->name, options->version));
  } else {
    // Add the version as a query parameter
    AZ_RETURN_IF_FAILED(
        az_http_request_builder_set_query_parameter(p_request, options->name, options->version));
  }
  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {
  (void)p_options;

  // TODO - add a UUID create implementation
  az_span uniqueid = AZ_SPAN_LITERAL_FROM_STR("123e4567-e89b-12d3-a456-426655440000");

  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  AZ_RETURN_IF_FAILED(az_http_request_append_header(p_request, AZ_MS_CLIENT_REQUESTID, uniqueid));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result
_az_http_policy_telemetry_options_init(_az_http_policy_telemetry_options * self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  *self = (_az_http_policy_telemetry_options){ 0 };
  self->os = AZ_STR("Unknown OS");
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request_builder * p_request,
    az_http_response * p_response) {

  _az_http_policy_telemetry_options * options = (_az_http_policy_telemetry_options *)(p_options);

  AZ_RETURN_IF_FAILED(
      az_http_request_append_header(p_request, AZ_HTTP_HEADER_USER_AGENT, options->os));

  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {

  _az_http_policy_apiversion_options * options = (_az_http_policy_apiversion_options *)(p_options);

  if (options->add_as_header) {
    // Add the version as a header
    AZ_RETURN_IF_FAILED(az_http_request_append_header(p_request, options->name, options->version));
  } else {
    // Add the version as a query parameter
    AZ_RETURN_IF_FAILED(
        az_http_request_set_query_parameter(p_request, options->name, options->version));
  }
  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {
  (void)p_options;

  // reset p_response to be written from the start
  AZ_RETURN_IF_FAILED(az_http_response_reset(p_response));

  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

typedef AZ_NODISCARD az_result (
    *_az_identity_auth_func)(void * p_options, az_http_request * p_request);

typedef struct {
  _az_identity_auth_func _func;
} _az_identity_auth;

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_options);

  _az_identity_auth * auth = (_az_identity_auth *)(p_options);
  AZ_CONTRACT_ARG_NOT_NULL(auth->_func);

  AZ_RETURN_IF_FAILED(auth->_func(p_options, p_request));

  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {

  if (az_log_should_write(AZ_LOG_HTTP_REQUEST)) {
    _az_log_http_request(p_request);
  }

  if (!az_log_should_write(AZ_LOG_HTTP_RESPONSE)) {
    // If no logging is needed, do not even measure the response time.
    return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
  }

  uint64_t const start = _az_clock_msec();
  az_result const result = az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
  uint64_t const end = _az_clock_msec();

  _az_log_http_response(p_response, end - start, p_request);

  return result;
}

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {
  (void)p_options;

  // buffer p_response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {
  (void)p_options;

  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response) {
  (void)p_policies; // this is the last policy in the pipeline, we just void it

  az_http_client client = *(az_http_client *)p_options;
  return client(p_request, p_response);
}
