// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_BUILDER_H
#define AZ_SPAN_BUILDER_H

#include <az_span.h>

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
  az_span buffer;
  /**
   * A current position in the buffer. The field is change after each write.
   */
  size_t position;
} az_span_builder;

/**
 * Creates a byte span builder.
 *
 * @buffer a buffer for writing. 
 */
AZ_INLINE az_span_builder az_span_builder_create(az_span const buffer) {
  return (az_span_builder){
    .buffer = buffer,
    .position = 0,
  };
}

/**
 * Returns a span of bytes that were written into the builder's buffer.
 */
AZ_INLINE az_span az_span_builder_result(az_span_builder const * const p_builder) {
  return az_span_take(p_builder->buffer, p_builder->position);
}

/**
 * Append a given span of bytes.
 */
az_result az_span_builder_append(az_span_builder * const p_builder, az_const_span const span);

AZ_CALLBACK_FUNC(az_span_builder_append, az_span_builder *, az_span_visitor)

#include <_az_cfg_suffix.h>

#endif
