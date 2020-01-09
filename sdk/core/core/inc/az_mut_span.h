// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MUT_SPAN_H
#define _az_MUT_SPAN_H

#include <az_action.h>
#include <az_contract.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_extern_include_prefix.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_extern_include_suffix.h>

#include <_az_cfg_prefix.h>

/**
 * A mutable span of bytes.
 */
typedef struct {
  uint8_t * begin;
  size_t size;
} az_mut_span;

AZ_NODISCARD AZ_INLINE az_mut_span az_mut_span_create_empty() { return (az_mut_span){ 0 }; }

AZ_NODISCARD AZ_INLINE bool az_mut_span_is_empty(az_mut_span const span) { return span.size == 0; }

AZ_NODISCARD AZ_INLINE bool az_mut_span_is_valid(az_mut_span const span) {
  return span.size == 0 || (span.begin != NULL && span.begin <= span.begin + span.size - 1);
}

/**
 * Cast the given mutable span to an immutable span.
 */
AZ_NODISCARD AZ_INLINE az_span az_mut_span_to_span(az_mut_span const span) {
  return (az_span){ .begin = span.begin, .size = span.size };
}

AZ_NODISCARD AZ_INLINE bool az_mut_spans_overlap(az_mut_span const a, az_mut_span const b) {
  return az_span_is_overlap(az_mut_span_to_span(a), az_mut_span_to_span(b));
}

AZ_NODISCARD AZ_INLINE az_result
az_mut_span_move(az_mut_span const buffer, az_span const src, az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  AZ_CONTRACT(buffer.size >= src.size, AZ_ERROR_BUFFER_OVERFLOW);

  if (!az_span_is_empty(src)) {
    memmove((void *)buffer.begin, (void const *)src.begin, src.size);
  }

  out_result->begin = buffer.begin;
  out_result->size = src.size;

  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_mut_span az_mut_span_drop(az_mut_span const span, size_t const n) {
  if (span.size <= n) {
    return az_mut_span_create_empty();
  }
  return (az_mut_span){ .begin = span.begin + n, .size = span.size - n };
}

AZ_NODISCARD AZ_INLINE az_mut_span az_mut_span_take(az_mut_span const span, size_t n) {
  if (span.size <= n) {
    return span;
  }
  return (az_mut_span){ .begin = span.begin, .size = n };
}

AZ_INLINE void az_mut_span_fill(az_mut_span const span, uint8_t const fill) {
  if (!az_mut_span_is_empty(span)) {
    memset(span.begin, fill, span.size);
  }
}

AZ_NODISCARD AZ_INLINE az_result
az_mut_span_set(az_mut_span const self, size_t const i, uint8_t const value) {
  if (self.size <= i) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  self.begin[i] = value;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_mut_span_to_str(az_mut_span const buffer, az_span const src, az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (buffer.size < src.size + 1) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_span_is_empty(src)) {
    az_mut_span result = { 0 };
    AZ_RETURN_IF_FAILED(az_mut_span_move(buffer, src, &result));
  }

  buffer.begin[src.size] = '\0';

  out_result->begin = buffer.begin;
  out_result->size = src.size + 1;

  return AZ_OK;
}

/**
 * ```c
 * typedef struct {
 *   az_result (* func)(void *, az_mut_span);
 *   void * self;
 * } az_mut_span_action;
 * ```
 *
 * Example of usage
 *
 * ```c
 * az_mut_span const span = ...;
 * az_mut_span_action const action = ...;
 * az_mut_span_action_do(action, span);
 * ```
 */
AZ_ACTION_TYPE(az_mut_span_action, az_mut_span)

#include <_az_cfg_suffix.h>

#endif
