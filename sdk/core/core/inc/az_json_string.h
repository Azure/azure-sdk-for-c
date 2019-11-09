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

#include <_az_cfg_suffix.h>

#endif
