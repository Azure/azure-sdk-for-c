// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_builder.h>

#include <_az_cfg.h>

az_result az_span_builder_append(az_span_builder * const p_builder, az_const_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(p_builder);

  az_span const remainder = az_span_drop(p_builder->buffer, p_builder->size);
  az_span result;
  AZ_RETURN_IF_FAILED(az_span_copy(remainder, span, &result));
  p_builder->size += result.size;
  return AZ_OK;
}
