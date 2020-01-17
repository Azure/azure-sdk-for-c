// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_INTERNAL_H
#define _az_LOG_INTERNAL_H

#include <az_http_policy.h>
#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_log.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef AZ_NODISCARD az_result (*az_log_policy)(
    az_result (*next_policy)(
        az_http_policy * const p_policies,
        az_http_request_builder * const hrb,
        az_http_response * const response),
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response * const response);

void az_log_write(az_log_classification const classification, az_span const message);

bool az_log_should_write(az_log_classification const classification);

AZ_NODISCARD az_result az_http_policy_log(
    az_result (*next_policy)(
        az_http_policy * const p_policies,
        az_http_request_builder * const hrb,
        az_http_response * const response),
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response * const response);

#include <_az_cfg_suffix.h>

#endif
