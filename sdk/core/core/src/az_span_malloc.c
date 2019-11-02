// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_malloc.h>

#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_malloc(size_t const size, az_mut_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  uint8_t * const p = (uint8_t *)malloc(size);
  if (p == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  *out = (az_mut_span){ .begin = p, .size = size };
  return AZ_OK;
}

void az_span_free(az_mut_span * const p) {
  if (p == NULL) {
    return;
  }
  free(p->begin);
  *p = (az_mut_span){ 0 };
}
