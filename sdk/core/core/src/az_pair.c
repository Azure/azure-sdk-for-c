// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_pair.h>

#include <_az_cfg.h>

AZ_FUNCTOR_DATA(az_pair_span_to_seq_data, az_pair_span const *, az_pair_seq)

az_result az_pair_span_to_seq_func(
    az_pair_span const * const context,
    az_pair_visitor const visitor) {
  AZ_CONTRACT_ARG_NOT_NULL(context);

  size_t const size = context->size;
  az_pair const * begin = context->begin;
  for (size_t i = 0; i < size; ++i) {
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, begin[i]));
  }
  return AZ_OK;
}

az_pair_seq az_pair_span_to_seq(az_pair_span const * const p_span) {
  return az_pair_span_to_seq_data(p_span, az_pair_span_to_seq_func);
}
