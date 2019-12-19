// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_builder.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_builder_append(az_span_builder * const self, az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_mut_span const remainder = az_mut_span_drop(self->buffer, self->size);
  az_mut_span result;
  AZ_RETURN_IF_FAILED(az_mut_span_move(remainder, span, &result));
  self->size += result.size;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_builder_append_byte(az_span_builder * const self, uint8_t const c) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_mut_span_set(self->buffer, self->size, c));
  self->size += 1;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_span_builder_append_zeros(az_span_builder * const self, size_t const size) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_mut_span const span = az_mut_span_take(az_mut_span_drop(self->buffer, self->size), size);
  if (span.size != size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  az_mut_span_memset(span, 0);
  return AZ_OK;
}
