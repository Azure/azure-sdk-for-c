// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_JSON_POINTER_H
#define _az_JSON_POINTER_H

#include <az_result.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * Returns a next reference token in the JSON pointer. The JSON pointer parser is @var
 * az_span_reader.
 *
 * See https://tools.ietf.org/html/rfc6901
 */
AZ_NODISCARD az_result
az_span_reader_read_json_pointer_token(az_span_reader * const self, az_span * const out);

/**
 * Returns a next character in the given span reader of JSON pointer reference token.
 */
AZ_NODISCARD az_result
az_span_reader_read_json_pointer_token_char(az_span_reader * const self, uint32_t * const out);

#include <_az_cfg_suffix.h>

#endif
