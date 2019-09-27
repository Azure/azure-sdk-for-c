// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STR_H
#define AZ_STR_H

#include <az_span.h>

#ifdef __cplusplus
extern "C" {
#endif

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""`
// to `S`.
#define AZ_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

#define AZ_STR_DECL(NAME, STRING_LITERAL) \
  az_const_span const NAME = {.begin = (uint8_t const *)STRING_LITERAL, \
                              .size = AZ_STRING_LITERAL_LEN(STRING_LITERAL)}

#define AZ_STR(STRING_LITERAL) \
  (az_const_span) { \
    .begin = (uint8_t const *)STRING_LITERAL, .size = AZ_STRING_LITERAL_LEN(STRING_LITERAL) \
  }

#ifdef __cplusplus
}
#endif

#endif
