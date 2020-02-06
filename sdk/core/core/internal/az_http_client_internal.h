// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_CLIENT_INTERNAL_H
#define _az_HTTP_CLIENT_INTERNAL_H

#include <az_http.h>

#ifdef MOCK_CURL
#include <az_mock_curl.h>
#else
#include <az_curl_adapter_internal.h>
#endif

#include <_az_cfg_prefix.h>

/**
 * @brief Passing http request to adapter. Adapter will handle GET, POST, PUT or DELETE
 * Respone is returned as RFC7230. Use az_http_response_parse to handle this format and get headers,
 * code and body.
 *
 * @param p_hrb pointer to method, headers, body and valid url to make a request
 * @param response pointer to span where response will be written
 * @return AZ_INLINE az_http_client_send_request
 */
AZ_NODISCARD AZ_INLINE az_result
az_http_client_send_request(az_http_request * const p_hrb, az_http_response * response) {
  return az_http_client_send_request_impl(p_hrb, response, true);
}

/**
 * @brief Passing http request to adapter. Adapter will handle GET, POST, PUT or DELETE
 * Response contains only the body
 *
 * @param p_hrb pointer to method, headers, body and valid url to make a request
 * @param response pointer to span where response will be written
 * @return AZ_INLINE az_http_client_send_request
 */
AZ_NODISCARD AZ_INLINE az_result az_http_client_send_request_and_get_body(
    az_http_request * p_hrb,
    az_http_response * response) {
  return az_http_client_send_request_impl(p_hrb, response, false);
}

#include <_az_cfg_suffix.h>

#endif
