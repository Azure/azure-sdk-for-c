// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_RESULT_H
#define AZ_RESULT_H

#include <stdbool.h>
#include <stdint.h>

#include <az_cfg_prefix.h>

enum {
  AZ_OK = 0,
  AZ_ERROR_FLAG = 0x80000000,
};

enum {
  AZ_CORE_FACILITY = 0x1,
  AZ_JSON_FACILITY = 0x2,
};

/**
 * The type represents error conditions.
 * Bits:
 * -  0..15 Code.
 * - 16..30 Facility.
 * - 31     Severity (0 - success, 1 - failure).
 */
typedef int32_t az_result;

#define AZ_MAKE_ERROR(facility, code) ((az_result)(0x80000000 | ((uint32_t)(facility) << 16)) | (uint32_t)(code))

#define AZ_MAKE_RESULT(facility, code) ((az_result)(((uint32_t)(facility) << 16)) | (uint32_t)(code))

inline bool az_failed(az_result result) { return (result & AZ_ERROR_FLAG) != 0; }

inline bool az_succeeded(az_result result) { return (result & AZ_ERROR_FLAG) == 0; }

#define AZ_RETURN_IF_NOT_OK(exp) \
  do { \
    az_result const result = (exp); \
    if (result != AZ_OK) { \
      return result; \
    } \
  } while (0)

enum {
  AZ_CORE_ERROR_ARGUMENT = AZ_MAKE_ERROR(AZ_CORE_FACILITY, 1),
};

#include <az_cfg_suffix.h>

#endif
