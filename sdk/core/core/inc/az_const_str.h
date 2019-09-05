// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CONST_STR_H
#define AZ_CONST_STR_H

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
} az_const_str;

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""` to `S`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

#define AZ_CONST_STR(STRING_LITERAL) \
  (az_const_str){ .begin = STRING_LITERAL, .size = AZ_STRING_LITERAL_SIZE(STRING_LITERAL) }

inline char const *az_const_str_ptr(az_const_str const buffer, size_t const index) {
  AZ_ASSERT(index <= buffer.size);
  return buffer.begin + index;
}

inline char az_const_str_item(az_const_str const buffer, size_t const index) {
  AZ_ASSERT(index < buffer.size);
  return buffer.begin[index];
}

inline char const *az_const_str_end(az_const_str const buffer) {
  return az_const_str_ptr(buffer, buffer.size);
}

inline size_t az_const_str_index(az_const_str const buffer, char const *p) {
  size_t const result = p - buffer.begin;
  AZ_ASSERT(result <= buffer.size);
  return result;
}

inline az_const_str az_const_str_from(az_const_str const buffer, size_t index) {
  return (az_const_str){ .begin = az_const_str_ptr(buffer, index), .size = buffer.size - index };
}

#ifdef __cplusplus
}
#endif

#endif
