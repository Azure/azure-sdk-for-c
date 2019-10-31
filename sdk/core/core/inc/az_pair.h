// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_PAIR_H
#define AZ_PAIR_H

#include <az_action.h>
#include <az_contract.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

/// A pair of strings.
typedef struct {
  az_span key;
  az_span value;
} az_pair;

/// A span of pairs.
typedef struct {
  az_pair const * begin;
  size_t size;
} az_pair_span;

/// @az_pair_visitor is a callback with one argument @az_pair.
AZ_ACTION_TYPE(az_pair_action, az_pair)

/// @az_pair_emitter is an emitter of @az_pair sequences.
AZ_ACTION_TYPE(az_pair_emitter, az_pair_action)

AZ_NODISCARD az_result
az_pair_span_emit(az_pair_span const * const self, az_pair_action const action);

/**
 * Creates @az_pair_span_emit_action from @az_pair_span.
 */
AZ_ACTION_FUNC(az_pair_span_emit, az_pair_span const, az_pair_emitter)

#include <_az_cfg_suffix.h>

#endif
