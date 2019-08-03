// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>

// Range

#define AZ_DEFINE_RANGE(TYPE, NAME) typedef struct { TYPE *begin; TYPE *end; } NAME;

#define AZ_ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

// String

AZ_DEFINE_RANGE(char const, az_string);

#define AZ_STRING_SIZE(S) (sizeof(S "") - 1)

#define _AZ_STRING(NAME, ARRAY, VALUE) \
  static char const ARRAY[] = VALUE; \
  static az_string NAME = { .begin = ARRAY, .end = ARRAY + AZ_STRING_SIZE(VALUE) }

#define AZ_DEFINE_STRING(NAME, VALUE) _AZ_STRING(NAME, _ ## NAME, VALUE)
