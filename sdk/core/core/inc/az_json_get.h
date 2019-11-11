// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_GET_H
#define AZ_JSON_GET_H

#include <az_json_parser.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result
az_json_get_object_member(az_span const json, az_span const name, az_json_value * const out_value);

/**
 * Get JSON value by JSON pointer https://tools.ietf.org/html/rfc6901.
 */
AZ_NODISCARD az_result
az_json_get_by_pointer(az_span const json, az_span const pointer, az_json_value * const out_value);

#include <_az_cfg_suffix.h>

#endif
