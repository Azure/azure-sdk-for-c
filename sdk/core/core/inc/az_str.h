// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STR_H
#define AZ_STR_H

#include <az_assert.h>

#include <az_static_assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

// An immutable string slice.
typedef struct {
  // Points to the first byte (character).
  uint8_t const *begin;
  // A numer of octets (bytes) in the string.
  size_t size;
} az_const_str;

// A muttable string slice.
typedef struct {
  uint8_t *begin;
  size_t size;
} az_str;

AZ_STATIC_ASSERT(CHAR_BIT == 8);

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""` to `S`.
#define AZ_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

#define AZ_CONST_STR_DECL(NAME, STRING_LITERAL) \
  static az_const_str const NAME = { .begin = STRING_LITERAL, .size = AZ_STRING_LITERAL_LEN(STRING_LITERAL) };

#define AZ_CONST_STR(STRING_LITERAL) \
  (az_const_str){ .begin = STRING_LITERAL, .size = AZ_STRING_LITERAL_LEN(STRING_LITERAL) }

inline uint8_t const *az_const_str_ptr(az_const_str const buffer, size_t const index) {
  AZ_ASSERT(index <= buffer.size);
  return buffer.begin + index;
}

inline uint8_t az_const_str_item(az_const_str const buffer, size_t const index) {
  AZ_ASSERT(index < buffer.size);
  return buffer.begin[index];
}

inline uint8_t const *az_const_str_end(az_const_str const buffer) {
  return az_const_str_ptr(buffer, buffer.size);
}

inline size_t az_const_str_index(az_const_str const buffer, uint8_t const *const p) {
  size_t const result = p - buffer.begin;
  AZ_ASSERT(result <= buffer.size);
  return result;
}

inline az_const_str az_const_substr(az_const_str const buffer, size_t const from, size_t const to) {
  AZ_ASSERT(from <= to);
  AZ_ASSERT(to <= buffer.size);
  return (az_const_str){ .begin = az_const_str_ptr(buffer, from), .size = to - from };
}

inline az_const_str az_to_const_str(az_str const str) {
  return (az_const_str) { .begin = str.begin, .size = str.size };
}

inline bool az_const_str_eq(az_const_str const a, az_const_str const b) {
  return a.size == b.size && memcmp(a.begin, b.begin, a.size) == 0;
}

#ifdef __cplusplus
}
#endif

#endif
