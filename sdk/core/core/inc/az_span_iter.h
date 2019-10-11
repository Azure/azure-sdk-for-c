// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_ITER_H
#define AZ_SPAN_ITER_H

#include <_az_cfg_prefix.h>

typedef struct az_span_iter az_span_iter;

typedef az_result (*az_span_iter_func)(az_span_iter * const p_i, az_span * const out);

struct az_span_iter {
  az_span_iter_func func;
  az_iter_data data;
};

static inline az_result az_span_iter_call(az_span_iter * const p_i, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);
  AZ_CONTRACT_ARG_NOT_NULL(p_i->func);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return p_i->func(p_i, out);
}

#include <_az_cfg_suffix.h>

#endif
