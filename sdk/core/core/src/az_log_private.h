// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_PRIVATE_H
#define _az_LOG_PRIVATE_H

#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

void _az_log_request(az_http_request_builder const * const hrb);

void _az_log_response(
    az_http_request_builder const * const hrb,
    az_http_response const * const response);

void _az_log_slow_response(
    az_http_request_builder const * const hrb,
    az_http_response const * const response,
    uint32_t duration_msec);

void _az_log_error(
    az_http_request_builder const * const hrb,
    az_http_response const * const response,
    az_result result);

#include <_az_cfg_suffix.h>

#endif
