// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_LOGGING_PRIVATE_H
#define _az_HTTP_POLICY_LOGGING_PRIVATE_H

#include <az_http_policy_internal.h>
#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result _az_http_policy_logging(az_http_policy_callback const next_policy);

void _az_http_policy_logging_log_http_request(az_http_request_builder const * const hrb);

void _az_http_policy_logging_log_http_response(
    az_http_response const * const response,
    uint64_t const duration_msec,
    az_http_request_builder const * const hrb);

#include <_az_cfg_suffix.h>

#endif
