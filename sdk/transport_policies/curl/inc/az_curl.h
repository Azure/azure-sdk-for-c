// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CURL_H
#define _az_CURL_H

#include <az_http.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result
_az_http_client_curl_send_request(_az_http_request * p_request, az_http_response * p_response);

#include <_az_cfg_suffix.h>

#endif
