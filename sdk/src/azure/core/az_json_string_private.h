// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_JSON_STRING_PRIVATE_H
#define _az_JSON_STRING_PRIVATE_H

#include <azure/core/az_json.h>
#include <azure/core/az_span.h>

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * TODO: this function and JSON pointer read functions should return proper UNICODE
 *       code-point to be compatible.
 */
AZ_NODISCARD az_result _az_span_reader_read_json_string_char(az_span* ref_span, uint32_t* out);

/**
 * Returns a next reference token in the JSON pointer. The JSON pointer parser is @var
 * az_span_reader.
 *
 * See https://tools.ietf.org/html/rfc6901
 */
AZ_NODISCARD az_result _az_span_reader_read_json_pointer_token(az_span* ref_span, az_span* out);

/**
 * Returns a next character in the given span reader of JSON pointer reference token.
 */
AZ_NODISCARD az_result
_az_span_reader_read_json_pointer_token_char(az_span* ref_span, uint32_t* out);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_JSON_STRING_PRIVATE_H
