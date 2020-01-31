// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_RETRY_PRIVATE_H
#define _az_HTTP_POLICY_RETRY_PRIVATE_H

#include <az_result.h>
#include <az_http_policy_internal.h>
#include <az_http_policy_retry_options.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result _az_http_policy_retry(
    az_http_policy_retry_options * options,
    az_http_policy_callback const next_policy);

#include <_az_cfg_suffix.h>

#endif
