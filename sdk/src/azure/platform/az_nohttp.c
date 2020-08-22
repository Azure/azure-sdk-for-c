// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_http_transport.h>

#include <azure/core/_az_cfg.h>

/**
 * @brief Provides no HTTP support.
 *
 * @param request An internal HTTP builder with data to build and send HTTP request.
 * @param ref_response A pre-allocated buffer where the HTTP response will be written.
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result
az_http_client_send_request(az_http_request const* request, az_http_response* ref_response)
{
  (void)request;
  (void)ref_response;
  return AZ_ERROR_NOT_IMPLEMENTED;
}
