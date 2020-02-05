// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_RESULT_H
#define _az_RESULT_H

#include <az_facility.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <_az_cfg_prefix.h>

enum {
  AZ_ERROR_FLAG = (int32_t)0x80000000,
};

#define _az_RESULT_MAKE_ERROR(facility, code) \
  ((int32_t)(0x80000000 | ((uint32_t)(facility) << 16) | (uint32_t)(code)))

#define AZ_MAKE_SUCCESS(facility, code) \
  ((int32_t)(0x00000000 | ((uint32_t)(facility) << 16) | (uint32_t)(code)))

#define AZ_RETURN_IF_FAILED(exp) \
  do { \
    az_result const _result = (exp); \
    if (az_failed(_result)) { \
      return _result; \
    } \
  } while (0)

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
typedef enum az_result {
  AZ_OK = AZ_MAKE_SUCCESS(AZ_FACILITY_CORE, 0),
  AZ_CONTINUE = AZ_MAKE_SUCCESS(AZ_FACILITY_CORE, 1),

  // Core
  AZ_ERROR_ARG = _az_RESULT_MAKE_ERROR(AZ_FACILITY_CORE, 1),
  AZ_ERROR_BUFFER_OVERFLOW = _az_RESULT_MAKE_ERROR(AZ_FACILITY_CORE, 2),
  AZ_ERROR_OUT_OF_MEMORY = _az_RESULT_MAKE_ERROR(AZ_FACILITY_CORE, 3),
  AZ_ERROR_NOT_IMPLEMENTED = _az_RESULT_MAKE_ERROR(AZ_FACILITY_CORE, 4),
  AZ_ERROR_ITEM_NOT_FOUND = _az_RESULT_MAKE_ERROR(AZ_FACILITY_CORE, 5),
  AZ_ERROR_PARSER_UNEXPECTED_CHAR = _az_RESULT_MAKE_ERROR(AZ_FACILITY_CORE, 6),

  // HTTP error codes
  AZ_ERROR_HTTP_PAL = _az_RESULT_MAKE_ERROR(AZ_FACILITY_HTTP, 1),
  AZ_ERROR_HTTP_INVALID_STATE = _az_RESULT_MAKE_ERROR(AZ_FACILITY_HTTP, 2),
  AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY = _az_RESULT_MAKE_ERROR(AZ_FACILITY_HTTP, 3),
  AZ_ERROR_HTTP_INVALID_METHOD_VERB = _az_RESULT_MAKE_ERROR(AZ_FACILITY_HTTP, 4),

  // JSON error codes
  AZ_ERROR_JSON_INVALID_STATE = _az_RESULT_MAKE_ERROR(AZ_FACILITY_JSON, 1),
  AZ_ERROR_JSON_STACK_OVERFLOW = _az_RESULT_MAKE_ERROR(AZ_FACILITY_JSON, 2),
  AZ_ERROR_JSON_STRING_END = _az_RESULT_MAKE_ERROR(AZ_FACILITY_JSON, 3),
  AZ_ERROR_JSON_POINTER_TOKEN_END = _az_RESULT_MAKE_ERROR(AZ_FACILITY_JSON, 4),

  // C standard errors
  AZ_ERROR_EOF = _az_RESULT_MAKE_ERROR(AZ_FACILITY_STD, 0xFFFF),
} az_result;

AZ_NODISCARD AZ_INLINE bool az_failed(az_result result) { return (result & AZ_ERROR_FLAG) != 0; }

AZ_NODISCARD AZ_INLINE bool az_succeeded(az_result result) { return (result & AZ_ERROR_FLAG) == 0; }

#include <_az_cfg_suffix.h>

#endif
