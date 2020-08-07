// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_LOGGING_PRIVATE_H
#define _az_HTTP_POLICY_LOGGING_PRIVATE_H

#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

void _az_http_policy_logging_log_http_request(az_http_request const* request);

void _az_http_policy_logging_log_http_response(
    az_http_response const* response,
    int64_t duration_msec,
    az_http_request const* request);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_HTTP_POLICY_LOGGING_PRIVATE_H
