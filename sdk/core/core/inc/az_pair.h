// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_PAIR_H
#define AZ_PAIR_H

#include <az_callback.h>
#include <az_contract.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

/// A pair of strings.
typedef struct {
  az_const_span key;
  az_const_span value;
} az_pair;

/// A span of pairs.
typedef struct {
  az_pair const * begin;
  size_t size;
} az_pair_span;

/// @az_pair_visitor is a callback with one argument @az_pair.
AZ_CALLBACK_TYPE(az_pair_visitor, az_pair)

/// @az_pair_seq is a @az_pair sequence visitor.
AZ_CALLBACK_TYPE(az_pair_seq, az_pair_visitor)

AZ_NODISCARD az_result
az_pair_span_to_seq(az_pair_span const * const context, az_pair_visitor const visitor);

/**
 * Creates @az_pair_seq from @az_pair_span.
 *
 * Note: the result of the function shouldn't be used after `*p_span` is destroyed.
 */
AZ_CALLBACK_FUNC(az_pair_span_to_seq, az_pair_span const *, az_pair_seq)

#include <_az_cfg_suffix.h>

#endif
