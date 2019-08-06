// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CORE_H
#define AZ_CORE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char const *begin;
  char const *end;
} az_cstr;

// String

// adding `""` to make sure that S is a `string literal`.
#define AZ_STRING_LITERAL_SIZE(S) (sizeof(S "") - 1)

#define _AZ_DEFINE_CSTR(NAME, ARRAY, VALUE) \
  static char const ARRAY[] = VALUE; \
  static az_cstr NAME = { .begin = ARRAY, .end = ARRAY + AZ_STRING_LITERAL_SIZE(VALUE) }

#define AZ_DEFINE_CSTR(NAME, VALUE) _AZ_DEFINE_CSTR(NAME, _ ## NAME, VALUE)

// Static assert

#define AZ_STATIC_ASSERT(C) typedef int _az_static_assert[(C) ? 1 : -1];

AZ_STATIC_ASSERT(true);

#ifdef __cplusplus
}
#endif

#endif
