// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_BASE64_H
#define _az_BASE64_H

#include <az_mut_span.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Encode as _Base64_: `Text` becomes `VGV4dA==`.
 *
 * @param base64url `true` if _base64url_ encoding should be used, `false` if plain _base64_ should
 * be used.
 * @param buffer a buffer to put the `out_result` in.
 * @param input an input to encode.
 * @param out_result the exact span inside the `buffer` where the encoded result is located in.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the result.
 * `buffer.size` that is _25%_larger_ than the `input.size` is always sufficient.
 *   - *`AZ_ERROR_ARG`*
 *     - `out_result` is _NULL_.
 *     - `buffer` or `input` are invalid spans (see @ref az_span_is_valid).
 *     - `out_result` would overlap `input`.
 */
AZ_NODISCARD az_result az_base64_encode(
    bool const base64url,
    az_mut_span const buffer,
    az_span const input,
    az_span * const out_result);

/**
 * @brief Decode from _Base64_: `VGV4dA==` becomes `Text`
 *
 * @param buffer a buffer to put the `out_result` in.
 * @param input an input to decode.
 * @param out_result the exact span inside the `buffer` where the decoded result is located in.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the result.
 * `buffer.size` that is _25%_smaller_ than `input.size` is always sufficient.
 *   - *`AZ_ERROR_ARG`*
 *     - `out_result` is _NULL_.
 *     - `buffer` or `input` are invalid spans (see @ref az_span_is_valid).
 *     - `input` is invalid: has _non-base64_ character (either plain _base64_, or _base64url_), has
 * _padding_ (`=` character) in the middle, or has invalid length (1, 5, 9, and so on).
 *     - `out_result` would overlap `input`.
 */
AZ_NODISCARD az_result
az_base64_decode(az_mut_span const buffer, az_span const input, az_span * const out_result);

#include <_az_cfg_suffix.h>

#endif
