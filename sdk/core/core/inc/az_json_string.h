// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STRING_H
#define AZ_JSON_STRING_H

#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

/**
 * - ..-1 is error.
 * - 0..0xFFFFF is a string character. Usually it's 0..0xFF UTF-8 byte but it can be an escape
 *   sequence `\uXXXX`.
 * - 0x10000 is an end of the string `"`.
 */
typedef enum {
  AZ_JSON_STRING_CHAR_END = 0x10000,
} az_json_string_char;

AZ_NODISCARD az_json_string_char az_span_reader_get_json_string_char(az_span_reader * const self);

#include <_az_cfg_suffix.h>

#endif
