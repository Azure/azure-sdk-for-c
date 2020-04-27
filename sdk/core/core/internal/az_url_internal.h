// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_URL_INTERNAL_H
#define _az_URL_INTERNAL_H

#include <az_config.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Copies character from the \p source #az_span to the \p destination #az_span by
 * URL-encoding the \p source span characters.
 *
 * @param[in] destination The #az_span whose bytes will receive the URL-encoded source.
 * @param[in] source The #az_span containing the non-URL-encoded bytes.
 * @param[out] out_length A pointer to a #uint32_t that is going to be assigned the length
 * of URL-encoding the \p source.
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the \p destination is not big enough to contain
 * the encoded bytes
 */
AZ_NODISCARD az_result _az_url_encode(az_span destination, az_span source, int32_t* out_length);

#include <_az_cfg_suffix.h>

#endif // _az_URL_INTERNAL_H
