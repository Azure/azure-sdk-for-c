// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "../inc/internal/az_contract.h"
#include <az_span_span.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_span_span_builder_append(az_span_span_builder * const self, az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  if (self->length >= self->buffer.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  self->buffer.begin[self->length] = span;

  self->length += 1;
  return AZ_OK;
}
