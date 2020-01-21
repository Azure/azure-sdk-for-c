// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_STR_H
#define _az_STR_H

#include <az_action.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""`
// to `S`.
#define AZ_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

/**
 * A callback that accepts zero-terminated string `char const *`.
 */
AZ_ACTION_TYPE(az_str_action, char const *)

#define AZ_CR '\r'

#define AZ_LF '\n'

#define AZ_CRLF "\r\n"

extern az_span const AZ_STR_ZERO;

#include <_az_cfg_suffix.h>

#endif
