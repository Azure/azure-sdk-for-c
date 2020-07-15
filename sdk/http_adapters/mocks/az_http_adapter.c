// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_http_transport.h>

#include <azure/core/_az_cfg.h>

#ifndef _az_MOCK_ENABLED
#error "Mock platform is only compatible with _az_MOCK_ENABLED"
#endif

#include <cmocka.h>

/**
 * @brief Provides mock implementation for testing.
 *
 * @param request An internal HTTP builder with data to build and send HTTP request.
 * @param ref_response A pre-allocated buffer where the HTTP response will be written.
 * @retval An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result
az_http_client_send_request(_az_http_request const* request, az_http_response* ref_response)
{
  void* fnc_send_request = (void*)mock();
  return fnc_send_request(request, ref_response);
}
