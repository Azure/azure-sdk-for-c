// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_contract_internal.h>
#include <az_span_malloc_internal.h>

#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_malloc(size_t size, az_span * out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  uint8_t * const p = (uint8_t *)malloc(size);
  if (p == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  *out = az_span_init(p, 0, size);
  return AZ_OK;
}

void az_span_free(az_span * p) {
  if (p == NULL) {
    return;
  }
  free(az_span_ptr(*p));
  *p = az_span_null();
}
