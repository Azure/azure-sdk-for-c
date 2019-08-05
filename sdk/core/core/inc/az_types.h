// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>

// Range

#define AZ_DEFINE_RANGE(TYPE, NAME) typedef struct { TYPE *begin; TYPE *const end; } NAME;

#define AZ_ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

// String

AZ_DEFINE_RANGE(char const, az_string);

AZ_DEFINE_RANGE(char, az_mutate_string)

#define AZ_STR_SIZE(S) (sizeof(S "") - 1)

#define _AZ_DEFINE_STR(NAME, ARRAY, VALUE) \
  static char const ARRAY[] = VALUE; \
  static az_string NAME = { .begin = ARRAY, .end = ARRAY + AZ_STR_SIZE(VALUE) }

#define AZ_DEFINE_STR(NAME, VALUE) _AZ_DEFINE_STR(NAME, _ ## NAME, VALUE)
