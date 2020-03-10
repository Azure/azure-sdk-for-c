// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_PRIVATE_H
#define _az_SPAN_PRIVATE_H

#include <az_precondition.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Use this only to create a span from uint8_t object.
 * The size of the returned span is always one.
 * Don't use this function for arrays. Use @var AZ_SPAN_FROM_ARRAY instead.
 * Don't us
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_single_item(uint8_t* ptr)
{
  return az_span_init(ptr, 1, 1);
}

/**
 * @brief move the content from span @b a to span @b b and viceverse.
 * The smallest span (less content) is swapped only. Example
 * a = 111
 * b = 22
 * after swaping
 * a = 221
 * b = 11
 *
 * @param a source/destination span
 * @param b destination/source span
 */
void _az_span_swap(az_span a, az_span b);

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
AZ_NODISCARD az_result _az_span_replace(az_span* self, int32_t start, int32_t end, az_span span);

typedef az_result (*_az_predicate)(az_span slice);

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result _az_scan_until(az_span self, _az_predicate predicate, int32_t* out_index);

AZ_NODISCARD az_result _az_is_expected_span(az_span* self, az_span expected);

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_PRIVATE_H
