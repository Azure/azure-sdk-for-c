// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_URI_H
#define AZ_URI_H

#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

/**
 * @brief _URI-Encode_ (a.k.a. _URL-Encode_): `this text` becomes `this%20text`.
 *
 * @param buffer a buffer to put the `out_result` in.
 * @param input an input to encode.
 * @param out_result the exact span inside the `buffer` where the encoded result is located in.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the result.
 * `buffer.size` that is _3_times_larger_ than the `input.size` is always sufficient.
 *   - *`AZ_ERROR_ARG`*
 *     - `out_result` is _NULL_.
 *     - `buffer` or `input` are invalid spans (see @ref az_span_is_valid).
 *     - `out_result` would overlap `input`.
 */
AZ_NODISCARD az_result
az_uri_encode(
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result);

/**
 * @brief _URI-Decode_ (a.k.a. _URL-Decode_): `this%20text` becomes `this text`.
 *
 * @param buffer a buffer to put the `out_result` in.
 * @param input an input to decode.
 * @param out_result the exact span inside the `buffer` where the decoded result is located in.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the result.
 * `buffer.size` that is _3_times_smaller_ than the `input.size` is always sufficient.
 *   - *`AZ_ERROR_ARG`*
 *     - `out_result` is _NULL_.
 *     - `buffer` or `input` are invalid spans (see @ref az_span_is_valid).
 *     - `input` is invalid: has a `%` character, that is not followed up by _two_ hexadecimal
 * (_`%[0-9A-Fa-f]{2}`_) characters: `%XZ`.
 *     - `out_result` would overlap `input`.
 */
AZ_NODISCARD az_result
az_uri_decode(
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result);

#include <_az_cfg_suffix.h>

#endif
