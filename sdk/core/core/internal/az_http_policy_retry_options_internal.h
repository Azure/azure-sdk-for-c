// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_RETRY_OPTIONS_INTERNAL_H
#define _az_HTTP_POLICY_RETRY_OPTIONS_INTERNAL_H

#include <az_http_policy_retry_options.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD bool az_http_policy_retry_options_validate(
    az_http_policy_retry_options const * retry_options);

#include <_az_cfg_suffix.h>

#endif
