// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_URI_INTERNAL_H
#define _az_URI_INTERNAL_H

#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

/**
 * @brief _URI-Encode_ (a.k.a. _URL-Encode_): `this text` becomes `this%20text`.
 *
 * @param input an input to encode.
 * @param span_builder Span builder to build the result to.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `span_builder` does not have enough space to fit the result.
 * `buffer.size` that is _3_times_larger_ than the `input.size` is always sufficient.
 *   - *`AZ_ERROR_ARG`*
 *     - `span_builder` is _NULL_.
 *     - `input` is invalid span (see @ref az_span_is_valid).
 *     - `span_builder`'s buffer would overlap `input`.
 */
AZ_NODISCARD az_result az_uri_encode(az_span input, az_span * const span_builder);

/**
 * @brief _URI-Decode_ (a.k.a. _URL-Decode_): `this%20text` becomes `this text`.
 *
 * @param input an input to decode.
 * @param span_builder Span builder to build the result to.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the result.
 * `buffer.size` that is _3_times_smaller_ than the `input.size` is always sufficient.
 *   - *`AZ_ERROR_ARG`*
 *     - `span_builder` is _NULL_.
 *     - `input` is invalid span (see @ref az_span_is_valid).
 *     - `input` is invalid: has a `%` character, that is not followed up by _two_ hexadecimal
 * (_`%[0-9A-Fa-f]{2}`_) characters: `%XZ`.
 *     - `span_builder`'s buffer would overlap `input`.
 */
AZ_NODISCARD az_result az_uri_decode(az_span input, az_span * const span_builder);

#include <_az_cfg_suffix.h>

#endif
