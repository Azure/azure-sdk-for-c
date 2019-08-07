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
  char const *begin;
  // Length (in chars) of the string.
  char const *end;
} az_cstr;

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are adding `""` to `S`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

// Defined a new constant string `NAME` which is equalent to the `STRING_LITERAL` value.
// For example:
//
// ```c
// AZ_CSTR(hello_world, "Hello world!");
//
// for (char const *i = hello_world.begin; i != hello_world.end; ++i) {
//   ...*i...
// }
// ```
#define _AZ_CSTR(NAME, ARRAY, VALUE) \
  static char const ARRAY[] = VALUE; \
  static az_cstr const NAME = { .begin = ARRAY, .end = ARRAY + AZ_STR_SIZE(VALUE) }

#define AZ_CSTR(NAME, VALUE) _AZ_DEFINE_STR(NAME, _ ## NAME, VALUE)

#ifdef __cplusplus
}
#endif

#endif
