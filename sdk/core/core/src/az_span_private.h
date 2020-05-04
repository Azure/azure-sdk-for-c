// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_PRIVATE_H
#define _az_SPAN_PRIVATE_H

#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Replace all contents from a starting position to an end position with the content of a
 * provided span
 *
 * @param self src span where to replace content
 * @param start starting position where to replace
 * @param end end position where to replace
 * @param span content to use for replacement
 * @return AZ_NODISCARD az_span_replace
 */
AZ_NODISCARD az_result
_az_span_replace(az_span self, int32_t self_length, int32_t start, int32_t end, az_span span);

typedef az_result (*_az_predicate)(az_span slice);

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result
_az_span_scan_until(az_span self, _az_predicate predicate, int32_t* out_index);

AZ_NODISCARD az_result _az_is_expected_span(az_span* self, az_span expected);

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

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_PRIVATE_H
