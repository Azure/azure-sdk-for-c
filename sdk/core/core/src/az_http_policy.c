// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_logging_private.h"
#include "az_http_policy_retry_private.h"
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_http_pipeline_nextpolicy(
    _az_http_policy* p_policies,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (p_policies[0]._internal.process == NULL)
  {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies[0]._internal.process(
      &(p_policies[1]), p_policies[0]._internal.p_options, p_request, p_response);
}

static AZ_NODISCARD az_result _az_http_pipeline_nextpolicy(
    _az_http_policy* policies,
    _az_http_request* ref_request,
    az_http_response* ref_response)
{
  return az_http_pipeline_nextpolicy(policies, ref_request, ref_response);
}

static const az_span AZ_MS_CLIENT_REQUESTID = AZ_SPAN_LITERAL_FROM_STR("x-ms-client-request-id");
static const az_span AZ_HTTP_HEADER_USER_AGENT = AZ_SPAN_LITERAL_FROM_STR("User-Agent");

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{

  _az_http_policy_apiversion_options* options = (_az_http_policy_apiversion_options*)(p_options);

  switch (options->_internal.option_location)
  {
    case _az_http_policy_apiversion_option_location_header:
      // Add the version as a header
      AZ_RETURN_IF_FAILED(az_http_request_append_header(
          p_request, options->_internal.name, options->_internal.version));
      break;
    case _az_http_policy_apiversion_option_location_queryparameter:
      // Add the version as a query parameter
      AZ_RETURN_IF_FAILED(az_http_request_set_query_parameter(
          p_request, options->_internal.name, options->_internal.version));
      break;
    default:
      return AZ_ERROR_ARG;
  }

  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_options;

  // TODO - add a UUID create implementation
  az_span const uniqueid = AZ_SPAN_LITERAL_FROM_STR("123e4567-e89b-12d3-a456-426655440000");

  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  AZ_RETURN_IF_FAILED(az_http_request_append_header(p_request, AZ_MS_CLIENT_REQUESTID, uniqueid));

  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{

  _az_http_policy_telemetry_options* options = (_az_http_policy_telemetry_options*)(p_options);

  AZ_RETURN_IF_FAILED(
      az_http_request_append_header(p_request, AZ_HTTP_HEADER_USER_AGENT, options->os));

  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  return _az_http_policy_retry(
    (az_http_policy_retry_options*)p_options,
    (az_http_policy_callback) {
      .func = _az_http_pipeline_nextpolicy,
      .params = {
        .policies = p_policies,
        .ref_request = p_request,
        .ref_response = p_response,
      },
    });
}

AZ_INLINE AZ_NODISCARD az_result
_az_apply_credential(_az_credential* credential, _az_http_request* ref_request)
{
  return (credential->_internal.apply_credential)(credential, ref_request);
}

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    _az_http_policy* policies,
    void* options,
    _az_http_request* ref_request,
    az_http_response* out_response)
{
  AZ_RETURN_IF_FAILED(_az_apply_credential((_az_credential*)options, ref_request));
  return az_http_pipeline_nextpolicy(policies, ref_request, out_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_options;

  return _az_http_policy_logging(
    (az_http_policy_callback) {
      .func = _az_http_pipeline_nextpolicy,
      .params = {
        .policies = p_policies,
        .ref_request = p_request,
        .ref_response = p_response,
      },
    });
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies; // this is the last policy in the pipeline, we just void it
  (void)p_options;

  return az_http_client_send_request(p_request, p_response);
}
