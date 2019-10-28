// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_seq.h>

#include <az_span_builder.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_span_span_to_seq(
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

AZ_NODISCARD az_result az_span_add_size(size_t * const p_size, az_const_span const span) {
  *p_size += span.size;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_seq_size(az_span_seq const seq, size_t * const out_size) {
  AZ_CONTRACT_ARG_NOT_NULL(out_size);

  *out_size = 0;
  return seq.func(seq.data, az_span_add_size_callback(out_size));
}

//

AZ_NODISCARD az_result az_span_seq_to_str(az_span_seq const seq, az_span const span) {
  az_span_builder i = az_span_builder_create(span);
  AZ_RETURN_IF_FAILED(seq.func(seq.data, az_span_builder_append_callback(&i)));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&i, AZ_STR("\0")));
  return AZ_OK;
}

//

AZ_NODISCARD az_result az_span_seq_to_new_str(az_span_seq const seq, char ** const out) { 
  *out = NULL;
  size_t size; 
  AZ_RETURN_IF_FAILED(az_span_seq_size(seq, &size));
  size += 1;
  uint8_t * const p = (uint8_t *)malloc(size);
  if (p == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  az_result const result = az_span_seq_to_str(seq, (az_span){ .begin = p, .size = size });
  if (az_failed(result)) {
    free(p);
    return result;
  }
  *out = (char *)p;
  return AZ_OK;
}
