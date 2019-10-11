// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_pair.h>

#include <_az_cfg.h>

az_result _az_pair_span_iter_func(az_pair_iter * const p_i, az_pair * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_pair const * const begin = p_i->data.begin;
  az_pair const * const end = p_i->data.end;
  if (begin == end) {
    return AZ_ERROR_EOF;
  }
  *out = *begin;
  p_i->data.begin = begin + 1;
  return AZ_OK;
}

az_pair_iter az_pair_span_to_iter(az_pair_span const span) {
  return (az_pair_iter){
    .func = _az_pair_span_iter_func,
    .data = { .begin = span.begin, .end = span.begin + span.size },
  };
}
