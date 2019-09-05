// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_RESULT_H
#define AZ_RESULT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_OK = 0,
  AZ_JSON_RESULT = 0x10000,
} az_result;

#define AZ_RETURN_ON_ERROR(exp) \
  do { \
    az_result const result = (exp); \
    if (result != AZ_OK) { \
      return result; \
    } \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif
