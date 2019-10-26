// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_seq.h>

#include <az_span_builder.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_span_span_to_seq(az_span_span const * const context, az_span_append const append) {
  AZ_CONTRACT_ARG_NOT_NULL(context);

  size_t const size = context->size;
  az_const_span const * begin = context->begin;
  for (size_t i = 0; i < size; ++i) {
    AZ_RETURN_IF_FAILED(az_span_append_do(append, begin[i]));
  }
  return AZ_OK;
}

// az_span_seq_size() and its utilities

AZ_CALLBACK_FUNC(az_span_add_size, size_t *, az_span_append)

AZ_NODISCARD az_result az_span_add_size(size_t * const p_size, az_const_span const span) {
  *p_size += span.size;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_seq_size(az_span_seq const seq, size_t * const out_size) {
  AZ_CONTRACT_ARG_NOT_NULL(out_size);

  *out_size = 0;
  return az_span_seq_do(seq, az_span_add_size_callback(out_size));
}

// az_span_seq_to_tmp_str() and its utilities

AZ_NODISCARD az_result
az_span_seq_to_str(az_span_seq const seq, az_mut_span const span, char const ** const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_span_builder i = az_span_builder_create(span);
  AZ_RETURN_IF_FAILED(az_span_seq_do(seq, az_span_builder_append_callback(&i)));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&i, AZ_STR("\0")));
  *out = (char const *)span.begin;
  return AZ_OK;
}

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

AZ_CALLBACK_TYPE(az_span_callback, az_mut_span)

AZ_NODISCARD az_result az_tmp_span(size_t const size, az_span_callback const callback) {
  az_mut_span span = { 0 };
  AZ_RETURN_IF_FAILED(az_span_malloc(size, &span));
  az_result const result = az_span_callback_do(callback, span);
  az_span_free(&span);
  return result;
}

typedef struct {
  az_span_seq seq;
  az_str_callback str_callback;
} az_span_callback_to_str_callback_data;

AZ_CALLBACK_FUNC(
    az_span_callback_to_str_callback,
    az_span_callback_to_str_callback_data const *,
    az_span_callback)

AZ_NODISCARD az_result az_span_callack_to_str_callback(
    az_span_callback_to_str_callback_data const * const p,
    az_mut_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  char const * str = NULL;
  AZ_RETURN_IF_FAILED(az_span_seq_to_str(p->seq, span, &str));
  AZ_RETURN_IF_FAILED(az_str_callback_do(p->str_callback, str));
  return AZ_OK;
}

AZ_NODISCARD az_result
az_span_seq_to_tmp_str(az_span_seq const seq, az_str_callback const callback) {
  size_t size = 0;
  AZ_RETURN_IF_FAILED(az_span_seq_size(seq, &size));
  {
    az_span_callback_to_str_callback_data data = {
      .seq = seq,
      .str_callback = callback,
    };
    AZ_RETURN_IF_FAILED(az_tmp_span(size, az_span_callback_to_str_callback_callback(&data)));
  }
  return AZ_OK;
}
