// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_uri.h>

#include <az_contract.h>

#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

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

AZ_NODISCARD AZ_INLINE uint8_t int4_to_hex(uint8_t const int4) {
  assert(int4 <= 0x0F);
  return (int4 < 10) ? ('0' + int4) : ('A' + (int4 - 10));
}

AZ_NODISCARD AZ_INLINE size_t encode(uint8_t const c, uint8_t * const p) {
  if (!should_encode(c)) {
    p[0] = c;
    return 1;
  }

  p[0] = '%';
  p[1] = int4_to_hex(c >> 4);
  p[2] = int4_to_hex(c & 0x0F);

  return 3;
}

AZ_NODISCARD az_result
az_uri_encode(
    az_mut_span const buffer,
    az_span const input,
    az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(input);

  size_t result_size = 0;
  for (size_t i = 0; i < input.size; ++i) {
    result_size += should_encode(input.begin[i]) ? 3 : 1;
  }

  assert(result_size <= buffer.size);
  az_mut_span const result = (az_mut_span){ .begin = buffer.begin, .size = result_size };

  if (az_span_is_overlap(input, az_mut_span_to_span(result))) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < result_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  uint8_t * p = buffer.begin;
  for (size_t i = 0; i < input.size; ++i) {
    assert(p < buffer.begin + result_size);
    p += encode(input.begin[i], p);
    assert(p <= buffer.begin + result_size);
  }

  *out_result = result;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_uri_decode(
    az_mut_span const buffer,
    az_span const input,
    az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(input);

  size_t result_size = 0;
  for (size_t i = 0; i < input.size; ++i) {
    uint8_t c = input.begin[i];
    if (c == '%') {
      if (i + 2 < i || i + 2 >= input.size || !isxdigit(input.begin[i + 1])
          || !isxdigit(input.begin[i + 2])) {
        return AZ_ERROR_ARG;
      }
      i += 2;
    }
    ++result_size;
  }

  az_mut_span const result = (az_mut_span){ .begin = buffer.begin, .size = result_size };

  if (az_span_is_overlap(input, az_mut_span_to_span(result))) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < result_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  for (size_t i = 0, j = 0; i < result_size; ++i, ++j) {
    assert(j < input.size);
    uint8_t c = input.begin[j];
    if (c == '%') {
      assert(j + 1 < input.size);
      assert(j + 2 < input.size);
      buffer.begin[i] = hex_to_int8(input.begin[j + 1], input.begin[j + 2]);
      j += 2;
    } else {
      buffer.begin[i] = c;
    }
  }

  *out_result = result;
  return AZ_OK;
}
