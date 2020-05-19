// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_transport.h>

#include <_az_cfg.h>

/**
 * @brief Provides no HTTP support.
 *
 * @param p_request An internal HTTP builder with data to build and send HTTP request.
 * @param p_response A pre-allocated buffer where the HTTP response will be written.
 * @retval An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result
az_http_client_send_request(_az_http_request* p_request, az_http_response* p_response)
{
  (void)p_request;
  (void)p_response;
  return AZ_ERROR_NOT_IMPLEMENTED;
}
