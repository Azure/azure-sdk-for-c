// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_result.h
 *
 * @brief az_result and facilities definition
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_RESULT_H
#define _az_RESULT_H

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

enum
{
  _az_FACILITY_CORE = 0x1,
  _az_FACILITY_PLATFORM = 0x2,
  _az_FACILITY_JSON = 0x3,
  _az_FACILITY_HTTP = 0x4,
  _az_FACILITY_MQTT = 0x5,
  _az_FACILITY_IOT = 0x6,
};

enum
{
  _az_ERROR_FLAG = (int32_t)0x80000000,
};

#define _az_RESULT_MAKE_ERROR(facility, code) \
  ((int32_t)(_az_ERROR_FLAG | ((int32_t)(facility) << 16) | (int32_t)(code)))

#define _az_RESULT_MAKE_SUCCESS(facility, code) \
  ((int32_t)(((int32_t)(facility) << 16) | (int32_t)(code)))

#define AZ_RETURN_IF_FAILED(exp) \
  do \
  { \
    az_result const _result = (exp); \
    if (az_failed(_result)) \
    { \
      return _result; \
    } \
  } while (0)

#define AZ_RETURN_IF_NOT_ENOUGH_SIZE(span, required_size) \
  do \
  { \
    if (az_span_size(span) < required_size || required_size < 0) \
    { \
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE; \
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
typedef enum
{
  // Core: Success results
  AZ_OK = _az_RESULT_MAKE_SUCCESS(_az_FACILITY_CORE, 0), ///< Success.
  AZ_CONTINUE = _az_RESULT_MAKE_SUCCESS(_az_FACILITY_CORE, 1),

  // Core: Error results
  AZ_ERROR_CANCELED = _az_RESULT_MAKE_ERROR(
      _az_FACILITY_CORE,
      0), ///< A context was canceled, and a function had to return before result was ready.

  AZ_ERROR_ARG = _az_RESULT_MAKE_ERROR(
      _az_FACILITY_CORE,
      1), ///< Input argument does not comply with the requested range of values.

  AZ_ERROR_INSUFFICIENT_SPAN_SIZE = _az_RESULT_MAKE_ERROR(
      _az_FACILITY_CORE,
      2), ///< The size of the provided span is too small.

  AZ_ERROR_NOT_IMPLEMENTED
  = _az_RESULT_MAKE_ERROR(_az_FACILITY_CORE, 3), ///< Requested functionality is not implemented.

  AZ_ERROR_ITEM_NOT_FOUND
  = _az_RESULT_MAKE_ERROR(_az_FACILITY_CORE, 4), ///< Requested item was not found.

  AZ_ERROR_PARSER_UNEXPECTED_CHAR
  = _az_RESULT_MAKE_ERROR(_az_FACILITY_CORE, 5), ///< Input can't be successfully parsed.

  AZ_ERROR_EOF = _az_RESULT_MAKE_ERROR(_az_FACILITY_CORE, 6), ///< Unexpected end of the input data.

  // Platform
  AZ_ERROR_MUTEX = _az_RESULT_MAKE_ERROR(_az_FACILITY_PLATFORM, 1), ///< Mutex operation error.

  AZ_ERROR_OUT_OF_MEMORY = _az_RESULT_MAKE_ERROR(
      _az_FACILITY_PLATFORM,
      2), ///< Dynamic memory allocation request was not successful.

  AZ_ERROR_HTTP_PLATFORM = _az_RESULT_MAKE_ERROR(
      _az_FACILITY_PLATFORM,
      3), ///< Generic error in the HTTP Client layer implementation.

  // JSON error codes
  AZ_ERROR_JSON_INVALID_STATE = _az_RESULT_MAKE_ERROR(_az_FACILITY_JSON, 1),
  AZ_ERROR_JSON_NESTING_OVERFLOW = _az_RESULT_MAKE_ERROR(_az_FACILITY_JSON, 2), ///< The JSON depth is too large.
  AZ_ERROR_JSON_STRING_END = _az_RESULT_MAKE_ERROR(_az_FACILITY_JSON, 3),
  AZ_ERROR_JSON_POINTER_TOKEN_END = _az_RESULT_MAKE_ERROR(_az_FACILITY_JSON, 4),

  // HTTP error codes
  AZ_ERROR_HTTP_INVALID_STATE = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 1),
  AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 2),
  AZ_ERROR_HTTP_INVALID_METHOD_VERB = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 3),

  AZ_ERROR_HTTP_AUTHENTICATION_FAILED
  = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 4), ///< Authentication failed.

  AZ_ERROR_HTTP_RESPONSE_OVERFLOW = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 5),
  AZ_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 6),

  AZ_ERROR_HTTP_CORRUPT_RESPONSE_HEADER = _az_RESULT_MAKE_ERROR(_az_FACILITY_HTTP, 7),

  // IoT error codes
  AZ_ERROR_IOT_TOPIC_NO_MATCH = _az_RESULT_MAKE_ERROR(_az_FACILITY_IOT, 1),
} az_result;

/// Checks wheteher the \a result provided indicates a failure.
///
/// @param result Result value to check for failure.
///
/// @retval true \a result indicates a failure.
/// @retval false \a result is successful.
AZ_NODISCARD AZ_INLINE bool az_failed(az_result result)
{
  return ((int32_t)result & (int32_t)_az_ERROR_FLAG) != 0;
}

/// Checks wheteher the \a result provided indicates a success.
///
/// @param result Result value to check for success.
///
/// @retval true \a result indicates success.
/// @retval false \a result is a failure.
AZ_NODISCARD AZ_INLINE bool az_succeeded(az_result result) { return !az_failed(result); }

#include <_az_cfg_suffix.h>

#endif // _az_RESULT_H
