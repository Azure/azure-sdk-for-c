// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STR_H
#define AZ_STR_H

#include <az_action.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""`
// to `S`.
#define AZ_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

/**
 * Creates a span which can be used inside constant initializers. Fro example
 *
 * `static az_const_span foo[] = { AZ_CONST_STR("Hello"), AZ_CONST_STR("world!") };`
 */
#define AZ_CONST_STR(STRING_LITERAL) \
  { .begin = (uint8_t const *)STRING_LITERAL, .size = AZ_STRING_LITERAL_LEN(STRING_LITERAL), }

// Creates a span which can be passed as a paramater. For example,
//
// `some_function(AZ_STR("Hello world"));`
//
// where
//
// `void some_function(az_const_span const span);`
#define AZ_STR(STRING_LITERAL) (az_const_span) AZ_CONST_STR(STRING_LITERAL)

/**
 * A callack that accepts zero-terminated string `char const *`.
 */
AZ_ACTION_TYPE(az_str_action, char const *)

#include <_az_cfg_suffix.h>

#endif
