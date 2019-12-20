// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_GET_H
#define AZ_JSON_GET_H

#include <az_json_token.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result
az_json_get_object_member(az_span const json, az_span const name, az_json_token * const out_token);

/**
 * Get JSON value by JSON pointer https://tools.ietf.org/html/rfc6901.
 */
AZ_NODISCARD az_result
az_json_get_by_pointer(az_span const json, az_span const pointer, az_json_token * const out_token);

#include <_az_cfg_suffix.h>

#endif
