// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CSTR_H
#define AZ_CSTR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A constant string.
typedef struct {
  // Points to the first character.
  char const *p;
  // Length (in chars) of the string.
  size_t len;
} az_cstr;

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are adding `""` to `S`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

// Defined a new constant string `NAME` which is equalent to the `STRING_LITERAL`. For example:
//
// ```c
// AZ_CSTR(hello_world, "Hello world!");
//
// for (size_t i = 0; i < hello_world.len; ++i) {
//   ...hello_world.p[i]...
// }
// ```
#define AZ_CSTR(NAME, STRING_LITERAL) \
  static az_cstr const NAME = { .begin = STRING_LITERAL, .size = AZ_STRING_LITERAL_SIZE(STRING_LITERAL) };

#ifdef __cplusplus
}
#endif

#endif
