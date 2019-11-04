// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_emitter.h>

#include <az_span_builder.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_span_span_emit(az_span_span const * const context, az_span_action const action) {
  AZ_CONTRACT_ARG_NOT_NULL(context);

  az_span const * i = context->begin;
  az_span const * const end = i + context->size;
  for (; i < end; ++i) {
    AZ_RETURN_IF_FAILED(az_span_action_do(action, *i));
  }
  return AZ_OK;
}

// az_span_emitter_size() and its utilities

AZ_NODISCARD az_result az_span_add_size(size_t * const p_size, az_span const span) {
  *p_size += span.size;
  return AZ_OK;
}

AZ_ACTION_FUNC(az_span_add_size, size_t, az_span_action)

AZ_NODISCARD az_result
az_span_emitter_size(az_span_emitter const emitter, size_t * const out_size) {
  AZ_CONTRACT_ARG_NOT_NULL(out_size);

  *out_size = 0;
  return az_span_emitter_do(emitter, az_span_add_size_action(out_size));
}

// az_span_emitter_to_tmp_str() and its utilities

AZ_NODISCARD az_result az_span_emitter_to_str(
    az_span_emitter const emitter,
    az_mut_span const span,
    char const ** const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_span_builder i = az_span_builder_create(span);
  AZ_RETURN_IF_FAILED(az_span_emitter_do(emitter, az_span_builder_append_action(&i)));
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

AZ_NODISCARD az_result az_tmp_span(size_t const size, az_mut_span_action const mut_span_action) {
  az_mut_span span;
  AZ_RETURN_IF_FAILED(az_span_malloc(size, &span));
  az_result const result = az_mut_span_action_do(mut_span_action, span);
  az_span_free(&span);
  return result;
}

typedef struct {
  az_span_emitter emitter;
  az_str_action str_action;
} az_str_action_to_mut_span_action_data;

AZ_ACTION_FUNC(
    az_str_action_to_mut_span_action,
    az_str_action_to_mut_span_action_data const,
    az_mut_span_action)

AZ_NODISCARD az_result az_str_action_to_mut_span_action(
    az_str_action_to_mut_span_action_data const * const self,
    az_mut_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  char const * str = NULL;
  AZ_RETURN_IF_FAILED(az_span_emitter_to_str(self->emitter, span, &str));
  AZ_RETURN_IF_FAILED(az_str_action_do(self->str_action, str));
  return AZ_OK;
}

AZ_NODISCARD az_result
az_span_emitter_to_tmp_str(az_span_emitter const emitter, az_str_action const str_action) {
  size_t size = 0;
  AZ_RETURN_IF_FAILED(az_span_emitter_size(emitter, &size));
  {
    az_str_action_to_mut_span_action_data data = {
      .emitter = emitter,
      .str_action = str_action,
    };
    AZ_RETURN_IF_FAILED(az_tmp_span(size, az_str_action_to_mut_span_action_action(&data)));
  }
  return AZ_OK;
}
