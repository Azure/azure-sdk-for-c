// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_POINTER_PARSER_H
#define AZ_JSON_POINTER_PARSER_H

#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

/**
 * Returns a next item in the JSON pointer. The JSON pointer parser is @az_span_reader.
 *
 * See https://tools.ietf.org/html/rfc6901
 */
AZ_NODISCARD az_result
az_json_pointer_parser_get(az_span_reader * const json_pointer_parser, az_span * const out);

#include <_az_cfg_suffix.h>

#endif
