// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CURL_ADAPTER_INTERNAL_H
#define _az_CURL_ADAPTER_INTERNAL_H

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <_az_cfg_prefix.h>

/**
 * Converts CURLcode to az_result.
 */
AZ_NODISCARD AZ_INLINE az_result az_curl_code_to_result(CURLcode code) {
  return code == CURLE_OK ? AZ_OK : AZ_ERROR_HTTP_PAL;
}

// returning AZ error on CURL Error
#define AZ_RETURN_IF_CURL_FAILED(exp) AZ_RETURN_IF_FAILED(az_curl_code_to_result(exp))

AZ_NODISCARD AZ_INLINE az_result az_curl_init(CURL ** out) {
  *out = curl_easy_init();
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_curl_done(CURL ** pp) {
  AZ_CONTRACT_ARG_NOT_NULL(pp);
  AZ_CONTRACT_ARG_NOT_NULL(*pp);

  curl_easy_cleanup(*pp);
  *pp = NULL;
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_client_send_request_impl(
    az_http_request * p_hrb,
    az_http_response * response,
    bool const buildRFC7230);

#include <_az_cfg_suffix.h>

#endif
