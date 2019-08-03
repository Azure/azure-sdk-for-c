// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>

#define AZ_STRING_SIZE(S) (sizeof(S "") - 1)

#define AZ_ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

#define AZ_DEFINE_SLICE(TYPE, NAME) typedef struct { TYPE *p; size_t size; } NAME

#define AZ_SLICE(A) { .p = (A), .size = AZ_ARRAY_SIZE(A) }

AZ_DEFINE_SLICE(char const, az_string);

#define AZ_STRING(S) { .p = (S), .size = AZ_STRING_SIZE(S) }
