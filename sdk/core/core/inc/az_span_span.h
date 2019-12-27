// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_SPAN_H
#define AZ_SPAN_SPAN_H

#include <az_result.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

/**
 * A span of spans of bytes.
 */
typedef struct {
  az_span const * begin;
  size_t size;
} az_span_span;

typedef struct {
  az_span * begin;
  size_t size;
} az_mut_span_span;

typedef struct {
  az_mut_span_span buffer;
  size_t length;
} az_span_span_builder;

/**
 * Creates a span of byte span builder.
 *
 * @param buffer a buffer for writing.
 */
AZ_NODISCARD AZ_INLINE az_span_span_builder
az_span_span_builder_create(az_mut_span_span const buffer) {
  return (az_span_span_builder){
    .buffer = buffer,
    .length = 0,
  };
}

/**
 * Append a given span.
 */
AZ_NODISCARD az_result
az_span_span_builder_append(az_span_span_builder * const self, az_span const span);

#include <_az_cfg_suffix.h>

#endif
