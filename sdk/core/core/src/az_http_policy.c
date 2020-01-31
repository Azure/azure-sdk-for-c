// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_log_private.h"
#include <az_clock_internal.h>
#include <az_http_client_internal.h>
#include <az_http_header_internal.h>
#include <az_http_pipeline.h>
#include <az_http_policy.h>
#include <az_http_request_builder.h>
#include <az_log.h>
#include <az_log_internal.h>
#include <az_mut_span.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_url_internal.h>

#include <stddef.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_http_pipeline_nextpolicy(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response * const response) {

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

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {

  _az_http_policy_apiversion_options * options = (_az_http_policy_apiversion_options *)(data);

  if (options->add_as_header) {
    // Add the version as a header
    AZ_RETURN_IF_FAILED(
        az_http_request_builder_append_header(hrb, options->name, options->version));
  } else {
    // Add the version as a query parameter
    AZ_RETURN_IF_FAILED(
        az_http_request_builder_set_query_parameter(hrb, options->name, options->version));
  }
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // TODO - add a UUID create implementation
  az_span const uniqueid = AZ_CONST_STR("123e4567-e89b-12d3-a456-426655440000");

  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_header(hrb, AZ_HTTP_HEADER_X_MS_CLIENT_REQUESTID, uniqueid));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result
_az_http_policy_telemetry_options_init(_az_http_policy_telemetry_options * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  *self = (_az_http_policy_telemetry_options){ 0 };
  self->os = AZ_STR("Unknown OS");
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {

  _az_http_policy_telemetry_options * options = (_az_http_policy_telemetry_options *)(data);

  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_header(hrb, AZ_HTTP_HEADER_USER_AGENT, options->os));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // reset response to be written from the start
  AZ_RETURN_IF_FAILED(az_http_response_reset(response));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

typedef AZ_NODISCARD az_result (
    *_az_identity_auth_func)(void * const data, az_http_request_builder * const hrb);

typedef struct {
  _az_identity_auth_func _func;
} _az_identity_auth;

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(data);

  _az_identity_auth const * const auth = (_az_identity_auth const *)(data);
  AZ_CONTRACT_ARG_NOT_NULL(auth->_func);

  AZ_RETURN_IF_FAILED(auth->_func(data, hrb));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;
  if (az_log_should_write(AZ_LOG_HTTP_REQUEST)) {
    _az_log_http_request(hrb);
  }

  if (!az_log_should_write(AZ_LOG_HTTP_RESPONSE)) {
    // If no logging is needed, do not even measure the response time.
    return az_http_pipeline_nextpolicy(p_policies, hrb, response);
  }

  uint64_t const start = _az_clock_msec();
  az_result const result = az_http_pipeline_nextpolicy(p_policies, hrb, response);
  uint64_t const end = _az_clock_msec();

  _az_log_http_response(response, end - start, hrb);

  return result;
}

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // Transport policy is the last policy
  //  If a policy exists after the transport policy
  if (p_policies[0].pfnc_process != NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return az_http_client_send_request(hrb, response);
}
