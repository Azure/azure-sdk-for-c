// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <_internal/az_uri.h>

#include <_internal/az_hex.h>
#include <az_contract.h>

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE bool should_encode(uint8_t const c) {
  switch (c) {
    case '-':
    case '_':
    case '.':
    case '~':
      return false;
    default:
      return !(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
  }
}

AZ_NODISCARD AZ_INLINE int8_t hex_to_int4(uint8_t const c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + ('9' - '0' + 1);
  }

  assert('a' <= c && c <= 'f');
  return c - 'a' + ('9' - '0' + 1);
}

AZ_NODISCARD AZ_INLINE uint8_t hex_to_int8(uint8_t const hi, uint8_t const lo) {
  return hex_to_int4(hi) << 4 | hex_to_int4(lo);
}

AZ_INLINE void encode(uint8_t const c, uint8_t * const p) {
  p[0] = '%';
  p[1] = az_number_to_upper_hex(c >> 4);
  p[2] = az_number_to_upper_hex(c & 0x0F);
}

AZ_NODISCARD az_result az_uri_encode(az_span const input, az_span_builder * const span_builder) {
  AZ_CONTRACT_ARG_NOT_NULL(span_builder);
  AZ_CONTRACT_ARG_VALID_SPAN(input);

  size_t const input_size = input.size;

  size_t result_size = 0;
  for (size_t i = 0; i < input_size; ++i) {
    result_size += should_encode(input.begin[i]) ? 3 : 1;
  }

  if (az_span_is_overlap(input, az_mut_span_to_span(span_builder->buffer))) {
    return AZ_ERROR_ARG;
  }

  if (span_builder->buffer.size < result_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  uint8_t const * p = input.begin;
  size_t s = 0;
  for (size_t i = 0; i < input_size; ++i) {
    uint8_t const c = input.begin[i];
    if (should_encode(c)) {
      if (s > 0) {
        AZ_RETURN_IF_FAILED(
            az_span_builder_append(span_builder, (az_span){ .begin = p, .size = s }));
      }

      uint8_t encoded[3];
      encode(c, encoded);
      AZ_RETURN_IF_FAILED(
          az_span_builder_append(span_builder, (az_span)AZ_SPAN_FROM_ARRAY(encoded)));

      p = input.begin + i + 1;
      s = 0;
    } else {
      ++s;
    }
  }

  if (s > 0) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(span_builder, (az_span){ .begin = p, .size = s }));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_uri_decode(az_span const input, az_span_builder * const span_builder) {
  AZ_CONTRACT_ARG_NOT_NULL(span_builder);
  AZ_CONTRACT_ARG_VALID_SPAN(input);

  size_t const input_size = input.size;

  size_t result_size = 0;
  for (size_t i = 0; i < input_size; ++i) {
    uint8_t c = input.begin[i];
    if (c == '%') {
      if (i + 2 < i || i + 2 >= input_size || !isxdigit(input.begin[i + 1])
          || !isxdigit(input.begin[i + 2])) {
        return AZ_ERROR_ARG;
      }
      i += 2;
    }
    ++result_size;
  }

  if (az_span_is_overlap(input, az_mut_span_to_span(span_builder->buffer))) {
    return AZ_ERROR_ARG;
  }

  if (span_builder->buffer.size < result_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  uint8_t const * p = input.begin;
  size_t s = 0;
  for (size_t i = 0; i < input_size; ++i) {
    uint8_t c = input.begin[i];
    if (c == '%') {
      if (s > 0) {
        AZ_RETURN_IF_FAILED(
            az_span_builder_append(span_builder, (az_span){ .begin = p, .size = s }));
      }

      assert(i + 1 < input.size);
      assert(i + 2 < input.size);

      AZ_RETURN_IF_FAILED(az_span_builder_append_byte(
          span_builder, hex_to_int8(input.begin[i + 1], input.begin[i + 2])));

      i += 2;
      p = input.begin + i + 1;
      s = 0;
    } else {
      ++s;
    }
  }

  if (s > 0) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(span_builder, (az_span){ .begin = p, .size = s }));
  }

  return AZ_OK;
}
