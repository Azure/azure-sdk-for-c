// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STRING_H
#define AZ_JSON_STRING_H

#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

/**
 * TODO: this function and JSON pointer read functions should return proper UNICODE
 *       code-point to be compatible.
 */
AZ_NODISCARD az_result
az_span_reader_read_json_string_char(az_span_reader * const self, uint32_t * const out);


/**
 * Encodes the given character into a JSON escape sequence. The function return an empty span if 
 * the given character doesn't require to be escaped.
 */
AZ_NODISCARD az_span az_json_esc_encode(az_result_byte const c);

enum {
  AZ_HEX_LOWER_OFFSET = 'a' - 10,
  AZ_HEX_UPPER_OFFSET = 'A' - 10,
};

/**
 * @digit should be in the range [0..15].
 */
AZ_NODISCARD AZ_INLINE uint8_t az_digit_to_upper_hex(uint8_t const digit) {
  return digit + (digit < 10 ? '0' : AZ_HEX_UPPER_OFFSET);
}

#include <_az_cfg_suffix.h>

#endif
