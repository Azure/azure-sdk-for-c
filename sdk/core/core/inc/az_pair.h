// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_PAIR_H
#define AZ_PAIR_H

#include <az_span.h>
#include <az_iter_data.h>
#include <az_contract.h>

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

/// A pair iterator.
typedef struct az_pair_iter az_pair_iter;

typedef az_result (*az_pair_iter_func)(az_pair_iter * const p_i, az_pair * const out);

struct az_pair_iter {
  az_pair_iter_func func;
  az_iter_data data;
};

static inline az_result az_pair_iter_call(az_pair_iter * const p_i, az_pair * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);
  AZ_CONTRACT_ARG_NOT_NULL(p_i->func);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return p_i->func(p_i, out);
}

/**
 * Creates an iterator of pairs from a span of pairs.
 */
az_pair_iter az_pair_span_to_iter(az_pair_span const span);

#include <_az_cfg_suffix.h>

#endif
