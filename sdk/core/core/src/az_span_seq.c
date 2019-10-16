// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_seq.h>

#include <_az_cfg.h>

//

AZ_CALLBACK_DATA(az_span_span_callback, az_span_span const *, az_span_seq)

az_result az_span_span_to_seq_func(
    az_span_span const * const context,
    az_span_visitor const visitor) {
  AZ_CONTRACT_ARG_NOT_NULL(context);

  size_t const size = context->size;
  az_const_span const * begin = context->begin;
  for (size_t i = 0; i < size; ++i) {
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, begin[i]));
  }
  return AZ_OK;
}

az_span_seq az_span_span_to_seq(az_span_span const * const p_span) {
  return az_span_span_callback(p_span, az_span_span_to_seq_func);
}

//

AZ_CALLBACK_DATA(az_size_callback, size_t *, az_span_visitor)

az_result az_span_add_size(size_t * const p_size, az_const_span const span) {
  *p_size += span.size;
  return AZ_OK;
}

az_result az_span_seq_size(az_span_seq const seq, size_t * const out_size) { 
  AZ_CONTRACT_ARG_NOT_NULL(out_size);

  *out_size = 0;
  return seq.func(seq.data, az_size_callback(out_size, az_span_add_size));
}
