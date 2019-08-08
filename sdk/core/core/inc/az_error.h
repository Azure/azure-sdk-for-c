// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_ERROR_H
#define AZ_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_OK = 0,
  AZ_JSON_ERROR = 0x10000,
} az_error;

#ifdef __cplusplus
}
#endif

#endif
