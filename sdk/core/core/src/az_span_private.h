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

/**
 * @brief Copies a URL in the \p source #az_span to the \p destination #az_span by url-encoding the
 * \p source span characters.
 *
 * @param[in] destination The #az_span whose bytes will receive the url-encoded source.
 * @param[in] source The #az_span containing the non-url-encoded bytes.
 * @param[out] out_span A pointer to an #az_span that receives the remainder of the \p destination
 * #az_span after the url-encoded \p source has been copied.
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the \p destination is not big enough to contain
 * the encoded bytes
 */
AZ_NODISCARD az_result
az_span_copy_url_encode(az_span destination, az_span source, az_span* out_span);

typedef az_result (*_az_predicate)(az_span slice);

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result _az_scan_until(az_span self, _az_predicate predicate, int32_t* out_index);

AZ_NODISCARD az_result _az_is_expected_span(az_span* self, az_span expected);

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_PRIVATE_H
