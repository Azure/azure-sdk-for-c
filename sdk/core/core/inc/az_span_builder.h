// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_BUILDER_H
#define _az_SPAN_BUILDER_H

#include <az_action.h>
#include <az_mut_span.h>
#include <az_span.h>
#include <az_span_action.h>
#include <az_span_internal.h>

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * A byte span builder.
 *
 * A builder is writing sequentially into a preallocated buffer.
 */
typedef struct {
  /**
   * A buffer for writting. This field is immutable.
   */
  az_mut_span buffer;
  /**
   * A current length in the resulting span. The field is increased after each write.
   */
  size_t length;
} az_span_builder;

/**
 * Creates a byte span builder.
 *
 * @param buffer a buffer for writing.
 */
AZ_NODISCARD AZ_INLINE az_span_builder az_span_builder_create(az_mut_span buffer) {
  return (az_span_builder){
    .buffer = buffer,
    .length = 0,
  };
}

AZ_INLINE void az_span_builder_reset(az_span_builder * self) {
  az_mut_span const buffer = self->buffer;
  az_mut_span_fill(buffer, 0);
  *self = az_span_builder_create(buffer);
}

/**
 * Returns a mutable span of bytes that were written into the builder's buffer.
 */
AZ_NODISCARD AZ_INLINE az_mut_span az_span_builder_mut_result(az_span_builder const * self) {
  return az_mut_span_take(self->buffer, self->length);
}

/**
 * Returns a span of bytes that were written into the builder's buffer.
 */
AZ_NODISCARD AZ_INLINE az_span az_span_builder_result(az_span_builder const * self) {
  return az_mut_span_to_span(az_span_builder_mut_result(self));
}

/**
 * Append a given span of bytes.
 */
AZ_NODISCARD az_result az_span_builder_append(az_span_builder * self, az_span span);

/**
 * Append a single byte.
 */
AZ_NODISCARD az_result az_span_builder_append_byte(az_span_builder * self, uint8_t c);

/**
 * Append zeros.
 */
AZ_NODISCARD az_result az_span_builder_append_zeros(az_span_builder * self, size_t size);

/**
 * Append an unsigned 64-bit integer (base 10).
 */
AZ_NODISCARD az_result az_span_builder_append_uint64(az_span_builder * self, uint64_t n);

/**
 * AppendAppend a signed 64-bit integer (base 10).
 */
AZ_NODISCARD az_result az_span_builder_append_int64(az_span_builder * self, int64_t n);

/**
 * Replace all contents from a starting position to an end position with the content of a provided
 * span
 */
AZ_NODISCARD az_result
az_span_builder_replace(az_span_builder * self, size_t start, size_t end, az_span span);

#include <_az_cfg_suffix.h>

#endif
