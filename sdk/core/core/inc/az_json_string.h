// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_JSON_STRING_H
#define _az_JSON_STRING_H

#include <az_result.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * TODO: this function and JSON pointer read functions should return proper UNICODE
 *       code-point to be compatible.
 */
AZ_NODISCARD az_result
az_span_reader_read_json_string_char(az_span_reader * const self, uint32_t * const out);

/**
 * Encodes the given character into a JSON escape sequence. The function returns an empty span if
 * the given character doesn't require to be escaped.
 */
AZ_NODISCARD az_span az_json_esc_encode(az_result_byte const c);

#include <_az_cfg_suffix.h>

#endif
