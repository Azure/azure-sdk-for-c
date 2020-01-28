// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_PRIVATE_H
#define _az_LOG_PRIVATE_H

#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

void _az_log_http_request(az_http_request_builder const * const hrb);

void _az_log_http_response(
    az_http_response const * const response,
    uint64_t const duration_msec,
    az_http_request_builder const * const hrb);

void _az_log_http_retry(
    int16_t const attempt,
    int32_t const delay_msec);

#include <_az_cfg_suffix.h>

#endif
