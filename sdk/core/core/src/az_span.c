// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include <az_span.h>

#include <ctype.h>

#include <_az_cfg.h>

enum {
  AZ_ASCII_LOWER_DIF = 'a' - 'A',
};

AZ_NODISCARD az_result az_span_sub(
    az_span const span,
    int32_t const low_index,
    int32_t high_index,
    az_span * out_sub_span) {
  // left part
  az_span const left = az_span_take(span, high_index);
  az_span const right = az_span_drop(left, low_index);
  AZ_RETURN_IF_FAILED(az_span_init(out_sub_span, right));

  return AZ_OK;
}

/**
 * ASCII lower case.
 */
AZ_NODISCARD AZ_INLINE az_result_byte az_ascii_lower(az_result_byte const value) {
  return 'A' <= value && value <= 'Z' ? value + AZ_ASCII_LOWER_DIF : value;
}

AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span const a, az_span const b) {
  int32_t const size = a._internal.length;
  if (size != b._internal.length) {
    return false;
  }
  for (int32_t i = 0; i < size; ++i) {
    if (az_ascii_lower(az_span_get(a, i)) != az_ascii_lower(az_span_get(b, i))) {
      return false;
    }
  }
  return true;
}

AZ_NODISCARD az_result az_span_to_uint64(az_span const self, uint64_t * const out) {
  if (self._internal.length <= 0) {
    return AZ_ERROR_EOF;
  }
  uint64_t value = 0;
  int32_t i = 0;
  while (true) {
    az_result_byte const result = az_span_get(self, i);
    if (result == AZ_ERROR_EOF) {
      *out = value;
      return AZ_OK;
    }
    if (!isdigit(result)) {
      return az_error_unexpected_char(result);
    }
    uint64_t const d = (uint64_t)result - '0';
    if ((UINT64_MAX - d) / 10 < value) {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
    value = value * 10 + d;
    i += 1;
  }
}

/****************** Mutating az_span (used to be az_mut_span in the origins)     ******/

/**
 * @brief move the content of span @b src to @buffer and make @b out_result point to it
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_mut_span_move
 */
AZ_NODISCARD az_result az_span_copy(az_span const dst, az_span const src, int32_t * out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  AZ_CONTRACT_ARG_VALID_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);
  int32_t src_len = az_span_length_get(src);

  AZ_CONTRACT(az_span_capacity_get(dst) >= src_len, AZ_ERROR_BUFFER_OVERFLOW);

  if (!az_span_is_empty(src)) {
    memmove((void *)az_span_prt_get(dst), (void const *)az_span_ptr_get(src), src_len);
  }

  out_result->_internal.begin = buffer._internal.begin;
  out_result->_internal.length = src._internal.length;
  out_result->_internal.capacity = src._internal.length;

  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE int32_t _az_size_min(int32_t const a, int32_t const b) {
  return a < b ? a : b;
}

AZ_INLINE void _az_uint8_swap(uint8_t * const a, uint8_t * const b) {
  uint8_t const c = *a;
  *a = *b;
  *b = c;
}

/**
 * @brief move the content from span @b a to span @b b and viceverse.
 * The smallest span (less content) is swapped only. Example
 * a = 111
 * b = 22
 * after swaping
 * a = 221
 * b = 11
 *
 * @param a source/destination span
 * @param b destination/source span
 */
void az_span_swap(az_span const a, az_span const b) {
  uint8_t * pa = a._internal.begin;
  uint8_t * pb = b._internal.begin;
  for (int32_t i = _az_size_min(a._internal.length, b._internal.length); i > 0; ++pa, ++pb) {
    --i;
    _az_uint8_swap(pa, pb);
  }
}

/**
 * @brief converts @b src span to zero-terminated srt. Content is copied to @b buffer and then \0 is
 * addeed at the end. Then out_result will be created out of buffer
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_mut_span_to_str
 */
AZ_NODISCARD az_result
az_span_to_str(az_span const buffer, az_span const src, az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (buffer._internal.capacity < src._internal.length + 1) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_span_is_empty(src)) {
    az_span result = { 0 };
    AZ_RETURN_IF_FAILED(az_mut_span_move(buffer, src, &result));
  }

  buffer._internal.begin[src._internal.length] = '\0';

  out_result->_internal.begin = buffer._internal.begin;
  out_result->_internal.length = src._internal.length + 1;
  out_result->_internal.capacity = src._internal.length + 1;

  return AZ_OK;
}

/****************** Building az_span (used to be az_span_builder in the origins) *****/

/**
 * @brief append az_span if there is enough capacity for it
 *
 * @param self src span where to append
 * @param span content to be appended
 * @return AZ_NODISCARD az_span_append
 */
AZ_NODISCARD az_result az_span_append(az_span * const self, az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span const remainder = az_span_drop(*self, self->_internal.length);
  az_span result;
  AZ_RETURN_IF_FAILED(az_mut_span_move(remainder, span, &result));
  self->_internal.length += result._internal.length;
  return AZ_OK;
}

/**
 * @brief Append only only byte if there is available capacity for it
 *
 * @param self src span where to append
 * @param c byte to be appended
 * @return AZ_NODISCARD az_span_append_byte
 */
AZ_NODISCARD az_result az_span_append_byte(az_span * const self, uint8_t const c) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_set(*self, self->_internal.length, c));
  self->_internal.length += 1;
  return AZ_OK;
}

/**
 * @brief Append @b size number of zeros to @b self if there is enough capacity for it
 *
 * @param self src span where to append
 * @param size number of zeros to be appended
 * @return AZ_NODISCARD az_span_append_zeros
 */
AZ_NODISCARD az_result az_span_append_zeros(az_span * const self, int32_t const size) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span const span = az_span_take(az_span_drop(*self, self->_internal.length), size);
  if (span._internal.capacity < size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  az_span_fill(span, 0);
  self->_internal.length += size;
  return AZ_OK;
}

/**
 * @brief Replace all contents from a starting position to an end position with the content of a
 * provided span
 *
 * @param self src span where to replace content
 * @param start starting position where to replace
 * @param end end position where to replace
 * @param span content to use for replacement
 * @return AZ_NODISCARD az_span_replace
 */
AZ_NODISCARD az_result
az_span_replace(az_span * const self, int32_t start, int32_t end, az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  int32_t const current_size = self->_internal.length;
  int32_t const replaced_size = end - start;
  int32_t const size_after_replace = current_size - replaced_size + span._internal.length;

  // replaced size must be less or equal to current builder size. Can't replace more than what
  // current is available
  AZ_CONTRACT(replaced_size <= current_size, AZ_ERROR_ARG);
  // start and end position must be before the end of current builder size
  AZ_CONTRACT(start <= current_size && end <= current_size, AZ_ERROR_ARG);
  // Start position must be less or equal than end position
  AZ_CONTRACT(start <= end, AZ_ERROR_ARG);
  // size after replacing must be less o equal than buffer size
  AZ_CONTRACT(size_after_replace <= self->_internal.length, AZ_ERROR_ARG);

  // get the span needed to be moved before adding a new span
  az_span const dst = az_span_drop(*self, start + span._internal.length);
  // get the span where to move content
  az_span const src = az_span_drop(*self, end);
  {
    // use a dummy result to use span_move
    az_span r = { 0 };
    // move content left or right so new span can be added
    AZ_RETURN_IF_FAILED(az_span_move(dst, src, &r));
    // add the new span
    AZ_RETURN_IF_FAILED(az_span_move(az_span_drop(*self, start), span, &r));
  }

  // update builder size
  self->_internal.length = size_after_replace;
  return AZ_OK;
}
