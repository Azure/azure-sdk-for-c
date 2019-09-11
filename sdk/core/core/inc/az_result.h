// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_RESULT_H
#define AZ_RESULT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_OK          = 0,
  AZ_ERROR_FLAG  = 0x80000000,
  AZ_JSON_RESULT =    0x10000,
};

typedef int32_t az_result;

#define AZ_MAKE_ERROR(facility, code) ((az_result)(0x80000000 | ((uint32_t)(facility) << 16)) | (uint32_t)(code))

#define AZ_MAKE_RESULT(facility, code) ((az_result)(((uint32_t)(facility) << 16)) | (uint32_t)(code))

inline bool az_failed(az_result result) {
  return result < 0;
}

inline bool az_succedded(az_result result) {
  return result >= 0;
}

#define AZ_RETURN_IF_NOT_OK(exp) \
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
