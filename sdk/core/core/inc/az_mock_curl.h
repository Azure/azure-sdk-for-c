// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_MOCK_CURL_H
#define AZ_MOCK_CURL_H

#include <_az_cfg.h>
#include <az_http_request.h>

#include <stdlib.h>

#include <_az_cfg_prefix.h>

AZ_INLINE az_result az_send_request_impl(az_http_request const * const p_request) {
  printf("TO BE IMPLEMENTED");
  return AZ_OK;
};

#include <_az_cfg_suffix.h>

#endif
