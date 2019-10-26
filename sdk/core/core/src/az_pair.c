// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_pair.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_pair_span_to_seq(
    az_pair_span const * const context,
    az_pair_append const append) {
  AZ_CONTRACT_ARG_NOT_NULL(context);

  size_t const size = context->size;
  az_pair const * begin = context->begin;
  for (size_t i = 0; i < size; ++i) {
    AZ_RETURN_IF_FAILED(az_pair_append_do(append, begin[i]));
  }
  return AZ_OK;
}
