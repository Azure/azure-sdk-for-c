// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CURL_ADAPTER_H
#define AZ_CURL_ADAPTER_H

#include <az_http_request.h>
#include <az_http_request_builder.h>
#include <az_span_builder.h>
#include <az_str.h>

#include <curl/curl.h>
#include <stdlib.h>

#include <_az_cfg_prefix.h>

// returning AZ error on CURL Error
#define AZ_RETURN_IF_CURL_FAILED(exp) \
  do { \
    CURLcode const _result = (exp); \
    if (_result != CURLE_OK) { \
      return AZ_ERROR_HTTP_PAL; \
    } \
  } while (0)

typedef struct {
  CURL * p_curl;
} az_curl;

AZ_NODISCARD AZ_INLINE az_result az_curl_init(az_curl * const out) {
  *out = (az_curl){
    .p_curl = curl_easy_init(),
  };
  curl_easy_setopt(out->p_curl, CURLOPT_FAILONERROR, 1);
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_curl_done(az_curl * const p) {
  AZ_CONTRACT_ARG_NOT_NULL(p);
  AZ_CONTRACT_ARG_NOT_NULL(p->p_curl);

  curl_easy_cleanup(p->p_curl);
  p->p_curl = NULL;
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_client_send_request_impl(
    az_http_request_builder * const p_hrb,
    az_mut_span const * const response);

#include <_az_cfg_suffix.h>

#endif
