// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_CLIENT_H
#define AZ_HTTP_CLIENT__H

#ifdef MOCK_CURL
#include <az_mock_curl.h>
#else
#include <az_curl_adapter.h>
#endif

#include <_az_cfg_prefix.h>

AZ_INLINE az_result az_http_client_send_request(az_http_request const * const p_request) {
  return az_send_request_impl(p_request);
}

#include <_az_cfg_suffix.h>

#endif