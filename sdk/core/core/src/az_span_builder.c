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

AZ_NODISCARD az_result az_span_builder_replace(
    az_span_builder * const self,
    size_t start,
    size_t end,
    az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(&start);
  AZ_CONTRACT_ARG_NOT_NULL(&end);

  // Get current builder size
  size_t const current_size = self->size;

  // Start position must be lower than current builder size
  AZ_CONTRACT(start <= current_size, AZ_ERROR_ARG);
  // Start position must be less or equal than end position
  AZ_CONTRACT(start <= end, AZ_ERROR_ARG);

  // Get the size of the replaced span
  size_t const replaced_size = end - start;

  // current content must be greater or equal to what we want to replace
  AZ_CONTRACT(current_size >= replaced_size, AZ_ERROR_ARG);

  size_t const required_shift = current_size - end;
  size_t const size_after_shift = current_size - replaced_size + span.size;

  // if replace size is different than new content size, shift content
  if (replaced_size != span.size) {
    if (replaced_size < span.size) {
      // validate theres enough space to shift right
      AZ_CONTRACT(size_after_shift <= self->buffer.size, AZ_ERROR_ARG);

      // shift right
      for (size_t i = 0; i < required_shift; ++i) {
        self->buffer.begin[size_after_shift - 1 - i] = self->buffer.begin[current_size - 1 - i];
      }

    } else {
      // shift left
      for (size_t i = 0; i < required_shift; ++i) {
        self->buffer.begin[start + span.size + i] = self->buffer.begin[end + i];
      }
    }
  }

  // override content
  self->size = start;
  AZ_RETURN_IF_FAILED(az_span_builder_append(self, span));
  self->size = size_after_shift;

  return AZ_OK;
}
