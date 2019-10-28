// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_MUT_SPAN_H
#define AZ_MUT_SPAN_H

#include <az_span.h>
#include <az_action.h>
#include <az_contract.h>
#include <az_result.h>
#include <az_static_assert.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

/**
 * A mutable span of bytes.
 */
typedef struct {
  uint8_t * begin;
  size_t size;
} az_mut_span;

typedef az_result az_result_byte;

AZ_INLINE bool az_mut_span_is_empty(az_mut_span const span) { return span.size <= 0; }

AZ_INLINE bool az_mut_span_is_valid(az_mut_span const span) {
  return span.size == 0 || (span.begin != NULL && span.begin <= span.begin + span.size - 1);
}

/**
 * Cast the given mutable span to an immutable span.
 */
AZ_INLINE az_span az_mut_span_to_span(az_mut_span const span) {
  return (az_span){ .begin = span.begin, .size = span.size };
}

AZ_INLINE bool az_mut_span_is_overlap(az_mut_span const a, az_mut_span const b) {
  return az_span_is_overlap(az_mut_span_to_span(a), az_mut_span_to_span(b));
}

AZ_INLINE az_result az_mut_span_copy(
    az_mut_span const buffer,
    az_span const src,
    az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_SPAN(src);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);

  if (az_span_is_overlap(az_mut_span_to_span(buffer), src)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < src.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_span_is_empty(src)) {
    memcpy((void *)buffer.begin, (void const *)src.begin, src.size);
  }

  out_result->begin = buffer.begin;
  out_result->size = src.size;

  return AZ_OK;
}

AZ_INLINE az_mut_span az_mut_span_drop(az_mut_span const span, size_t const n) {
  if (span.size <= n) {
    return (az_mut_span){ .begin = NULL, .size = 0 };
  }
  return (az_mut_span){ .begin = span.begin + n, .size = span.size - n };
}

AZ_INLINE az_mut_span az_mut_span_take(az_mut_span const span, size_t n) {
  if (span.size <= n) {
    return span;
  }
  return (az_mut_span){ .begin = span.begin, .size = n };
}

AZ_INLINE az_result az_mut_span_set(az_mut_span const span, uint8_t const fill) {
  if (!az_mut_span_is_valid(span)) {
    return AZ_ERROR_ARG;
  }

  if (!az_mut_span_is_empty(span)) {
    memset(span.begin, fill, span.size);
  }

  return AZ_OK;
}

AZ_INLINE az_result az_mut_span_swap(az_mut_span const a, az_mut_span const b) {
  if (!az_mut_span_is_valid(a) || !az_mut_span_is_valid(b)) {
    return AZ_ERROR_ARG;
  }

  if (a.size != b.size || az_mut_span_is_overlap(a, b)) {
    return AZ_ERROR_ARG;
  }

  if (!az_mut_span_is_empty(a)) {
    uint8_t * a_ptr = a.begin;
    uint8_t * b_ptr = b.begin;
    uint8_t const * const a_end = a.begin + a.size;
    do {
      uint8_t const old_a = *a_ptr;
      *a_ptr = *b_ptr;
      *b_ptr = old_a;

      ++a_ptr;
      ++b_ptr;
    } while (a_ptr != a_end);
  }

  return AZ_OK;
}

AZ_INLINE az_result az_mut_span_to_str(
    az_mut_span const buffer,
    az_span const src,
    az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (buffer.size < src.size + 1) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_span_is_empty(src)) {
    memmove((void *)buffer.begin, (void const *)src.begin, src.size);
  }

  buffer.begin[src.size] = '\0';

  out_result->begin = buffer.begin;
  out_result->size = src.size + 1;

  return AZ_OK;
}

AZ_NODISCARD az_result az_mut_span_replace(
    az_mut_span const buffer,
    az_span const src,
    uint8_t (*const func)(uint8_t const),
    az_mut_span * const out_result);

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
