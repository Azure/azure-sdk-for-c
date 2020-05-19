// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_private.h"
#include "az_http_private.h"
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_span.h>

#include <_az_cfg.h>

static const az_span AZ_HTTP_HEADER_USER_AGENT = AZ_SPAN_LITERAL_FROM_STR("User-Agent");

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response)
{

  _az_http_policy_apiversion_options* options = (_az_http_policy_apiversion_options*)(p_data);

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

AZ_INLINE AZ_NODISCARD az_result
_az_apply_credential(_az_credential* credential, _az_http_request* ref_request)
{
  // Only apply the credential if the apply_credential method exists
  return (credential->_internal.apply_credential == NULL)
      ? AZ_OK
      : (credential->_internal.apply_credential)(credential, ref_request);
}

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  if (p_data != AZ_CREDENTIAL_ANONYMOUS)
  {
    AZ_RETURN_IF_FAILED(_az_apply_credential((_az_credential*)p_data, p_request));
  }
  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies; // this is the last policy in the pipeline, we just void it
  (void)p_data;

  // make sure the response is resetted
  _az_http_response_reset(p_response);

  return az_http_client_send_request(p_request, p_response);
}
