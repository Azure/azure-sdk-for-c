// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MOCK_CURL_INTERNAL_H
#define _az_MOCK_CURL_INTERNAL_H

#include <az_http_request_internal.h>
#include <az_http_response.h>
#include <az_result.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_result az_http_client_send_request_impl(
    az_http_request const * const p_request,
    az_http_response * const response,
    bool allow_allocate) {
  return AZ_ERROR_NOT_IMPLEMENTED;
};

#include <_az_cfg_suffix.h>

#endif
