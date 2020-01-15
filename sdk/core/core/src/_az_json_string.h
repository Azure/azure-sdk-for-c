// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef __az_JSON_STRING_H
#define __az_JSON_STRING_H

#include "_az_span.h"
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * Encodes the given character into a JSON escape sequence. The function returns an empty span if
 * the given character doesn't require to be escaped.
 */
AZ_NODISCARD az_span az_json_esc_encode(az_result_byte const c);

#include <_az_cfg_suffix.h>

#endif
