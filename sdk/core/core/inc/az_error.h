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

#define AZ_RETURN_ON_ERROR(exp) \
  do { \
    az_error const error = (exp); \
    if (error != AZ_OK) { \
      return error; \
    } \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif
