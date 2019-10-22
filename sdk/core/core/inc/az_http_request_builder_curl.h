// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_BUILDER_CURL_H
#define AZ_HTTP_REQUEST_BUILDER_CURL_H

#include <az_http_request_builder.h>

#include <az_curl_adapter.h>

#include <_az_cfg_prefix.h>

az_result az_http_request_builder_to_curl(
    az_http_request_builder const * const p_builder,
    az_curl * const out);

#include <_az_cfg_suffix.h>

#endif
