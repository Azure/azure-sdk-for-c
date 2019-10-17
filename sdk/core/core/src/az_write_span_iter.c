// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_write_span_iter.h>

#include <_az_cfg.h>

az_result az_write_span_iter_write(az_write_span_iter * const p_i, az_const_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);

  az_span const remainder = az_span_drop(p_i->span, p_i->i);
  az_span result;
  AZ_RETURN_IF_FAILED(az_span_copy(remainder, span, &result));
  p_i->i += result.size;
  return AZ_OK;
}
