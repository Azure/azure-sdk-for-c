// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_URI_H
#define AZ_URI_H

#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

az_result az_uri_encode(
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result);

az_result az_uri_decode(
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result);

#include <_az_cfg_suffix.h>

#endif
