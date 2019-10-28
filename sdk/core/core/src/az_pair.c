// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_pair.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_pair_span_emit(
    az_pair_span const * const self,
    az_pair_action const action) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_pair const * i = self->begin;
  az_pair const * const end = i + self->size;
  for (; i < end; ++i) {
    AZ_RETURN_IF_FAILED(az_pair_action_do(action, *i));
  }
  return AZ_OK;
}
