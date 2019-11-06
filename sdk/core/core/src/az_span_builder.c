// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_builder.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_builder_append(az_span_builder * const self, az_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_mut_span const remainder = az_mut_span_drop(self->buffer, self->size);
  az_mut_span result;
  AZ_RETURN_IF_FAILED(az_mut_span_copy(remainder, span, &result));
  self->size += result.size;
  return AZ_OK;
}
