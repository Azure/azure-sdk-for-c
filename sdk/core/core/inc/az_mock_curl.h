// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_MOCK_CURL_H
#define AZ_MOCK_CURL_H

#include <az_http_request.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_INLINE az_result az_send_request_impl(
    az_http_request const * const p_request,
    az_span * const response,
    bool allow_allocate) {
  return AZ_ERROR_NOT_IMPLEMENTED;
};

#include <_az_cfg_suffix.h>

#endif
