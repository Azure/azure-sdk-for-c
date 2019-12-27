// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_builder.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_builder_append(az_span_builder * const self, az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_mut_span const remainder = az_mut_span_drop(self->buffer, self->length);
  az_mut_span result;
  AZ_RETURN_IF_FAILED(az_mut_span_move(remainder, span, &result));
  self->length += result.size;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_builder_append_byte(az_span_builder * const self, uint8_t const c) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_mut_span_set(self->buffer, self->length, c));
  self->length += 1;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_builder_replace(
    az_span_builder * const self,
    size_t start,
    size_t end,
    az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  size_t const current_size = self->length;
  size_t const replaced_size = end - start;
  size_t const size_after_replace = current_size - replaced_size + span.size;

  // replaced size must be less or equal to current builder size. Can't replace more than what
  // current is available
  AZ_CONTRACT(replaced_size <= current_size, AZ_ERROR_ARG);
  // start and end position must be before the end of current builder size
  AZ_CONTRACT(start <= current_size && end <= current_size, AZ_ERROR_ARG);
  // Start position must be less or equal than end position
  AZ_CONTRACT(start <= end, AZ_ERROR_ARG);
  // size after replacing must be less o equal than buffer size
  AZ_CONTRACT(size_after_replace <= self->buffer.size, AZ_ERROR_ARG);

  // get the span then need to be moved before adding new span
  az_mut_span const dst = az_mut_span_drop(self->buffer, start + span.size);
  // get the span where to move content
  az_span const src = az_span_drop(az_span_builder_result(self), end);
  {
    // use a dummy result to use span_move
    az_mut_span r = { 0 };
    // move content left or right so new span can be added
    AZ_RETURN_IF_FAILED(az_mut_span_move(dst, src, &r));
    // add the new span
    AZ_RETURN_IF_FAILED(az_mut_span_move(az_mut_span_drop(self->buffer, start), span, &r));
  }

  // update builder size
  self->length = size_after_replace;
  return AZ_OK;
}
