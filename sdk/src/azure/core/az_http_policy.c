// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_private.h"
#include <azure/core/az_credentials.h>
#include <azure/core/az_http.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/az_version.h>
#include <azure/core/internal/az_http_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    _az_http_policy* ref_policies,
    void* ref_options,
    az_http_request* ref_request,
    az_http_response* ref_response)
{

  _az_http_policy_apiversion_options const* const options
      = (_az_http_policy_apiversion_options const*)ref_options;

  switch (options->_internal.option_location)
  {
    case _az_http_policy_apiversion_option_location_header:
      // Add the version as a header
      _az_RETURN_IF_FAILED(az_http_request_append_header(
          ref_request, options->_internal.name, options->_internal.version));
      break;
    case _az_http_policy_apiversion_option_location_queryparameter:
      // Add the version as a query parameter. This value doesn't need url-encoding. Use `true` for
      // url-encode to avoid encoding.
      _az_RETURN_IF_FAILED(az_http_request_set_query_parameter(
          ref_request, options->_internal.name, options->_internal.version, true));
      break;
    default:
      return AZ_ERROR_ARG;
  }

  return _az_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    _az_http_policy* ref_policies,
    void* ref_options,
    az_http_request* ref_request,
    az_http_response* ref_response)
{
  _az_PRECONDITION_NOT_NULL(ref_options);

  // Format spec: https://azure.github.io/azure-sdk/general_azurecore.html#telemetry-policy
  uint8_t telemetry_id_buffer[200] = "azsdk-c-";
  az_span telemetry_id = AZ_SPAN_FROM_BUFFER(telemetry_id_buffer);
  {
    az_span remainder = az_span_slice_to_end(telemetry_id, sizeof("azsdk-c-") - 1);

    _az_http_policy_telemetry_options* options = (_az_http_policy_telemetry_options*)(ref_options);
    az_span const component_name = options->component_name;
    _az_PRECONDITION_VALID_SPAN(component_name, 1, false);
    remainder = az_span_copy(remainder, component_name);

    remainder = az_span_copy_u8(remainder, '/');
    remainder = az_span_copy(remainder, AZ_SPAN_FROM_STR(AZ_SDK_VERSION_STRING));

    telemetry_id = az_span_slice(
        telemetry_id, 0, (int32_t)(az_span_ptr(remainder) - az_span_ptr(telemetry_id)));
  }

  _az_RETURN_IF_FAILED(
      az_http_request_append_header(ref_request, AZ_SPAN_FROM_STR("User-Agent"), telemetry_id));

  return _az_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    _az_http_policy* ref_policies,
    void* ref_options,
    az_http_request* ref_request,
    az_http_response* ref_response)
{
  _az_credential* const credential = (_az_credential*)ref_options;
  _az_http_policy_process_fn const policy_credential_apply
      = credential == NULL ? NULL : credential->_internal.apply_credential_policy;

  if (credential == AZ_CREDENTIAL_ANONYMOUS || policy_credential_apply == NULL)
  {
    return _az_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
  }

  return policy_credential_apply(ref_policies, credential, ref_request, ref_response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    _az_http_policy* ref_policies,
    void* ref_options,
    az_http_request* ref_request,
    az_http_response* ref_response)
{
  (void)ref_policies; // this is the last policy in the pipeline, we just void it
  (void)ref_options;

  // make sure the response is resetted
  _az_http_response_reset(ref_response);

  return az_http_client_send_request(ref_request, ref_response);
}
