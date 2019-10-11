// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_WRITE_SPAN_ITER_H
#define AZ_WRITE_SPAN_ITER_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span span;
  size_t i;
} az_write_span_iter;

AZ_INLINE az_write_span_iter az_write_span_iter_create(az_span const span) {
  return (az_write_span_iter){
    .span = span,
    .i = 0,
  };
}

AZ_INLINE az_span az_write_span_iter_result(az_write_span_iter const * const p_i) {
  return az_span_take(p_i->span, p_i->i);
}

AZ_INLINE az_result
az_write_span_iter_write(az_write_span_iter * const p_i, az_const_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);

  az_span const remainder = az_span_drop(p_i->span, p_i->i);
  AZ_RETURN_IF_FAILED(az_span_copy(span, remainder));
  p_i->i += span.size;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
