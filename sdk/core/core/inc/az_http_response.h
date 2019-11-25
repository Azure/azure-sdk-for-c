// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_H
#define AZ_HTTP_RESPONSE_H

#include <az_mut_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_mut_span value;
} az_http_response;

AZ_NODISCARD AZ_INLINE az_result
az_http_response_init(az_http_response * const self, az_mut_span const value) {
  self->value = value;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
