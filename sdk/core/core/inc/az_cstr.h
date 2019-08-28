// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CSTR_H
#define AZ_CSTR_H

#include <az_assert.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A constant string.
typedef struct {
  // Points to the first character.
  char const *begin;
  // A numer of C characters (bytes) in the string.
  size_t size;
} az_cstr;

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are adding `""` to `S`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

#define AZ_CSTR(STRING_LITERAL) \
  ((az_cstr){ .begin = STRING_LITERAL, .size = AZ_STRING_LITERAL_SIZE(STRING_LITERAL) })

inline char const *az_cstr_ptr(az_cstr const buffer, size_t const index) {
  AZ_ASSERT(index <= buffer.size);
  return buffer.begin + index;
}

inline char az_cstr_item(az_cstr const buffer, size_t const index) {
  AZ_ASSERT(index < buffer.size);
  return buffer.begin[index];
}

inline char const *az_cstr_end(az_cstr const buffer) {
  return az_cstr_ptr(buffer, buffer.size);
}

inline size_t az_cstr_index(az_cstr const buffer, char const *p) {
  size_t const result = p - buffer.begin;
  AZ_ASSERT(result <= buffer.size);
  return result;
}

inline az_cstr az_cstr_from(az_cstr const buffer, size_t index) {
  return (az_cstr){ .begin = az_cstr_ptr(buffer, index), .size = buffer.size - index };
}

#ifdef __cplusplus
}
#endif

#endif
