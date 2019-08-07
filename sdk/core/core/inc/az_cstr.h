// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CSTR_H
#define AZ_CSTR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// A constant string.
// The string is empty if `begin` is equal to `end`.
typedef struct {
  // Points to the first character.
  char const *begin;
  // Points to the character after the last character.
  char const *end;
} az_cstr;

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are adding `""` to `S`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

#define _AZ_CSTR(NAME, ARRAY, STRING_LITERAL) \
  static char const ARRAY[] = STRING_LITERAL; \
  static az_cstr NAME = { .begin = ARRAY, .end = ARRAY + AZ_STRING_LITERAL_SIZE(STRING_LITERAL) };

// Defined a new constant string `NAME` which is equalent to the `STRING_LITERAL`. For example:
//
// ```c
// AZ_CSTR(hello_world, "Hello world!");
// ```
#define AZ_CSTR(NAME, STRING_LITERAL) _AZ_CSTR(NAME, _ ## NAME, STRING_LITERAL)

#ifdef __cplusplus
}
#endif

#endif
