// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_BASE64_H
#define AZ_BASE64_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

az_result az_base64_encode(
    bool const base64url,
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result);

az_result az_base64_decode(
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result);

#include <_az_cfg_suffix.h>

#endif
