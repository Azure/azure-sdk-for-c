// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_AAD_PRIVATE_H
#define _az_AAD_PRIVATE_H

#include <az_config.h>
#include <az_config_internal.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

enum
{
  _az_AAD_REQUEST_URL_BUF_SIZE = AZ_HTTP_REQUEST_URL_BUF_SIZE,
  _az_AAD_REQUEST_HEADER_BUF_SIZE = 10 * sizeof(az_pair),
  _az_AAD_REQUEST_BODY_BUF_SIZE = AZ_HTTP_REQUEST_BODY_BUF_SIZE,
  _az_AAD_RESPONSE_BUF_SIZE = 3 * 1024,
};

AZ_NODISCARD az_result _az_aad_build_url(az_span url, az_span tenant_id, az_span* out_url);

AZ_NODISCARD az_result _az_aad_build_body(
    az_span body,
    az_span client_id,
    az_span scopes,
    az_span client_secret,
    az_span* out_body);

AZ_NODISCARD az_result _az_aad_request_token(_az_http_request* request, _az_token* out_token);

AZ_NODISCARD bool _az_token_expired(_az_token const* token);

AZ_NODISCARD az_result _az_token_set(_az_token* self, _az_token const* new_token);

#include <_az_cfg_suffix.h>

#endif // _az_AAD_PRIVATE_H
