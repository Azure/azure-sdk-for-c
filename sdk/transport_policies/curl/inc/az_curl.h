// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CURL_H
#define _az_CURL_H

#include <az_http.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result az_http_transport_options_init(az_http_transport_options * out_options);

#include <_az_cfg_suffix.h>

#endif
