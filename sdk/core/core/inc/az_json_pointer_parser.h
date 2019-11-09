// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_POINTER_PARSER_H
#define AZ_JSON_POINTER_PARSER_H

#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

/**
 * Returns a next reference token in the JSON pointer. The JSON pointer parser is @az_span_reader.
 *
 * See https://tools.ietf.org/html/rfc6901
 */
AZ_NODISCARD az_result az_span_reader_read_json_pointer_token(
    az_span_reader * const json_pointer_parser,
    az_span * const out);

AZ_NODISCARD az_result az_span_reader_read_json_pointer_token_char(
    az_span_reader * const json_pointer_token_parser,
    uint8_t * const out);

#include <_az_cfg_suffix.h>

#endif
