// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_CLIENT_H
#define AZ_HTTP_CLIENT_H

#ifdef MOCK_CURL
#include <az_mock_curl.h>
#else
#include <az_curl_adapter.h>
#endif

#include <_az_cfg_prefix.h>

/**
 * @brief Passing http request to adapter. Adapter will handle GET, POST, PUT or DELETE
 *
 * @param p_hrb pointer to method, headers, body and valid url to make a request
 * @param response pointer to span where response will be written
 * @return AZ_INLINE az_http_client_send_request
 */
AZ_NODISCARD AZ_INLINE az_result
az_http_client_send_request(az_http_request_builder * const p_hrb, az_span const * const response) {
  return az_http_client_send_request_impl(p_hrb, response);
}

#include <_az_cfg_suffix.h>

#endif
