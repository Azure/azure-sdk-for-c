// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_OPTION_BYTE_H
#define AZ_OPTION_BYTE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int16_t az_option_byte;

enum {
  AZ_OPTION_BYTE_NONE = EOF,
};

#ifdef __cplusplus
}
#endif

#endif
