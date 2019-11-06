// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_GET_H
#define AZ_JSON_GET_H

#include <az_json_parser.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result
az_json_get_object_member(az_span const json, az_span const name, az_json_value * const out_value);

/*
AZ_NODISCARD az_result
az_json_get_object_member_string(az_span const json, az_span const name, az_span * const out_value);

AZ_NODISCARD az_result
az_json_get_object_member_number(az_span const json, az_span const name, double * const out_value);

AZ_NODISCARD az_result
az_json_get_object_member_boolean(az_span const json, az_span const name, bool * const out_value);
*/

#include <_az_cfg_suffix.h>

#endif
