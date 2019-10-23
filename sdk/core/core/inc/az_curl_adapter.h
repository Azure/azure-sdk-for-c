// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CURL_ADAPTER_H
#define AZ_CURL_ADAPTER_H

#include <az_callback.h>
#include <az_http_request.h>
#include <az_span_seq.h>
#include <az_span_builder.h>

#include <curl/curl.h>
#include <stdlib.h>

#include <_az_cfg_prefix.h>

typedef struct {
  CURL * p_curl;
} az_curl;

AZ_INLINE az_result az_curl_init(az_curl * const out) {
  *out = (az_curl){
    .p_curl = curl_easy_init(),
  };
  curl_easy_setopt(out->p_curl, CURLOPT_FAILONERROR, 1);
  return AZ_OK;
}

AZ_INLINE az_result az_curl_done(az_curl * const p) {
  AZ_CONTRACT_ARG_NOT_NULL(p);
  AZ_CONTRACT_ARG_NOT_NULL(p->p_curl);

  curl_easy_cleanup(p->p_curl);
  p->p_curl = NULL;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
