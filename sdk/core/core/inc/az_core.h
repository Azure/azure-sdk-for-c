// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>

#define DEFINE_SLICE(TYPE, NAME) typedef struct { size_t const size; TYPE const *p; } NAME

DEFINE_SLICE(char, STRING);

DEFINE_SLICE(int, INT_SLICE);

#define STRING_SIZE(S) (sizeof(S) - 1)

#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

#define CREATE_STRING(S) { .size = STRING_SIZE(S), .p = (S) }

#define CREATE_SLICE(A) { .size = ARRAY_SIZE(A), .p = (A) }
