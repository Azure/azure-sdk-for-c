// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CURL_ADAPTER_H
#define AZ_CURL_ADAPTER_H

#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>

#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <_az_cfg_prefix.h>

/**
 * Converts CURLcode to az_result.
 */
AZ_NODISCARD AZ_INLINE az_result az_curl_code_to_result(CURLcode const code) {
  return code == CURLE_OK ? AZ_OK : AZ_ERROR_HTTP_PAL;
}

// returning AZ error on CURL Error
#define AZ_RETURN_IF_CURL_FAILED(exp) AZ_RETURN_IF_FAILED(az_curl_code_to_result(exp))

AZ_NODISCARD AZ_INLINE az_result az_curl_init(CURL ** const out) {
  *out = curl_easy_init();
  return AZ_OK;
}

AZ_NODISCARD az_result az_curl_done(CURL ** const pp);

AZ_NODISCARD az_result az_http_client_send_request_impl(
    az_http_request_builder * const p_hrb,
    az_http_response * const response,
    bool const buildRFC7230);

#include <_az_cfg_suffix.h>

#endif
