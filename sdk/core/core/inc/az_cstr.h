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
  // A numer of C characters (bytes) in the string.
  size_t len;
} az_cstr;

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are adding `""` to `S`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

// Defines a new constant string `NAME` which points to the `STRING_LITERAL` value.
#define AZ_CSTR(VALUE) { .p = VALUE, .len = AZ_STRING_LITERAL_SIZE(VALUE) }

#ifdef __cplusplus
}
#endif

#endif
