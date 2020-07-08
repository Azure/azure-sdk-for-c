// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_PRIVATE_H
#define _az_SPAN_PRIVATE_H

#include <azure/core/az_precondition.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <stdbool.h>

#include <azure/core/_az_cfg_prefix.h>

enum
{
  _az_ASCII_LOWER_DIF = 'a' - 'A',

  // One less than the number of digits in _az_MAX_SAFE_INTEGER
  // This many digits can roundtrip between double and uint64_t without loss of precision
  // or causing integer overflow. We can't choose 16, because 9999999999999999 is larger than
  // _az_MAX_SAFE_INTEGER.
  _az_MAX_SUPPORTED_FRACTIONAL_DIGITS = 15,
};

/**
 * @brief Replace all contents from a starting position to an end position with the content of a
 * provided span
 *
 * @param destination src span where to replace content
 * @param start starting position where to replace
 * @param end end position where to replace
 * @param replacement content to use for replacement
 * @return AZ_NODISCARD az_span_replace
 */
AZ_NODISCARD az_result _az_span_replace(
    az_span destination,
    int32_t current_size,
    int32_t start,
    int32_t end,
    az_span replacement);

typedef az_result (*_az_predicate)(az_span slice);

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result
_az_span_scan_until(az_span span, _az_predicate predicate, int32_t* out_index);

AZ_NODISCARD az_result _az_is_expected_span(az_span* ref_span, az_span expected);

/**
 * @brief Removes all leading and trailing white space characters from the \p span. Function will
 * create a new #az_span pointing to the first non-white-space (` `, \\n, \\r, \\t) character found
 * in \p span and up to the last non-white-space character.
 *
 * @remarks If \p span is full of non-white-space characters, this function will return empty
 * #az_span.
 *
 * Example:
 * \code{.c}
 *  az_span a = AZ_SPAN_FROM_STR("  text with   \\n spaces   ");
 *  az_span b = _az_span_trim_white_space(a);
 *  // assert( b ==  AZ_SPAN_FROM_STR("text with   \\n spaces"));
 * \endcode
 *
 * @param[in] source #az_span pointing to a memory address that might contain white spaces.
 * @return The trimmed #az_span.
 */
AZ_NODISCARD az_span _az_span_trim_white_space(az_span source);

/**
 * @brief Removes all leading white space characters from the start of \p span.
 * Function will create a new #az_span pointing to the first non-white-space (` `, \\n, \\r, \\t)
 * character found in \p span and up to the last character.
 *
 * @remarks If \p span is full of non-white-space characters, this function will return empty
 * #az_span.
 *
 * Example:
 * \code{.c}
 *  az_span a = AZ_SPAN_FROM_STR("  text with   \\n spaces   ");
 *  az_span b = _az_span_trim_white_space_from_start(a);
 *  // assert( b ==  AZ_SPAN_FROM_STR("text with   \\n spaces   "));
 * \endcode
 *
 * @param[in] source #az_span pointing to a memory address that might contain white spaces.
 * @return The trimmed #az_span.
 */
AZ_NODISCARD az_span _az_span_trim_white_space_from_start(az_span source);

/**
 * @brief Removes all trailing white space characters from the end of \p span.
 * Function will create a new #az_span pointing to the first character in \p span and up to the last
 * non-white-space (` `, \\n, \\r, \\t) character.
 *
 * @remarks If \p span is full of non-white-space characters, this function will return empty
 * #az_span.
 *
 * Example:
 * \code{.c}
 *  az_span a = AZ_SPAN_FROM_STR("  text with   \\n spaces   ");
 *  az_span b = _az_span_trim_white_space_from_end(a);
 *  // assert( b ==  AZ_SPAN_FROM_STR("  text with   \\n spaces"));
 * \endcode
 *
 * @param[in] source #az_span pointing to a memory address that might contain white spaces.
 * @return The trimmed #az_span.
 */
AZ_NODISCARD az_span _az_span_trim_white_space_from_end(az_span source);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_SPAN_PRIVATE_H
