// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>

#define AZ_DEFINE_SLICE(TYPE, NAME) typedef struct { size_t size; TYPE const *p; } NAME

AZ_DEFINE_SLICE(char, az_string);

AZ_DEFINE_SLICE(int, az_int_slice);

#define AZ_STRING_SIZE(S) (sizeof(S) - 1)

#define AZ_ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

#define AZ_STRING(S) { .size = AZ_STRING_SIZE(S), .p = (S) }

#define AZ_SLICE(A) { .size = AZ_ARRAY_SIZE(A), .p = (A) }
