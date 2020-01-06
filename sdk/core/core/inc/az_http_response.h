// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_H
#define AZ_HTTP_RESPONSE_H

#include <az_result.h>
#include <az_span_builder.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span_builder builder;
} az_http_response;

AZ_NODISCARD AZ_INLINE az_result
az_http_response_init(az_http_response * const self, az_span_builder const builder) {
  self->builder = builder;
  return AZ_OK;
}

/**
 * @brief Sets length of builder to zero so builder's buffer can be written from start
 *
 */
AZ_NODISCARD AZ_INLINE az_result az_http_response_reset(az_http_response * const self) {
  self->builder.length = 0;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
