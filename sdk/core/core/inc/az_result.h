// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_RESULT_H
#define AZ_RESULT_H

#include <az_static_assert.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <_az_cfg_prefix.h>

/**
 * The type represents error conditions.
 * Bits:
 * - 31 Severity (0 - success, 1 - failure).
 * - if failure then
 *   - 16..30 Facility.
 *   -  0..15 Code.
 * - otherwise
 *   -  0..30 Value
 */
typedef int32_t az_result;

enum {
  AZ_OK = 0,
  AZ_ERROR_FLAG = (az_result)0x80000000,
};

enum {
  AZ_CORE_FACILITY = 0x1,
  AZ_JSON_FACILITY = 0x2,
  AZ_HTTP_FACILITY = 0x3,
  AZ_STD_FACILITY = 0x7FFF,
};

#define AZ_MAKE_ERROR(facility, code) \
  ((az_result)(0x80000000 | ((uint32_t)(facility) << 16) | (uint32_t)(code)))

AZ_NODISCARD AZ_INLINE bool az_failed(az_result result) { return (result & AZ_ERROR_FLAG) != 0; }

AZ_NODISCARD AZ_INLINE bool az_succeeded(az_result result) { return (result & AZ_ERROR_FLAG) == 0; }

#define AZ_RETURN_IF_FAILED(exp) \
  do { \
    az_result const _result = (exp); \
    if (az_failed(_result)) { \
      return _result; \
    } \
  } while (0)

enum {
  AZ_ERROR_ARG = AZ_MAKE_ERROR(AZ_CORE_FACILITY, 1),
  AZ_ERROR_BUFFER_OVERFLOW = AZ_MAKE_ERROR(AZ_CORE_FACILITY, 2),
  AZ_ERROR_OUT_OF_MEMORY = AZ_MAKE_ERROR(AZ_CORE_FACILITY, 3),
  AZ_ERROR_NOT_IMPLEMENTED = AZ_MAKE_ERROR(AZ_CORE_FACILITY, 4),

  AZ_ERROR_HTTP_FAILED_REQUEST = AZ_MAKE_ERROR(AZ_HTTP_FACILITY, 1),

  AZ_ERROR_EOF = AZ_MAKE_ERROR(AZ_STD_FACILITY, 0xFFFF),
};

AZ_STATIC_ASSERT(AZ_ERROR_EOF == EOF)
AZ_STATIC_ASSERT(EOF == -1)

#include <_az_cfg_suffix.h>

#endif
