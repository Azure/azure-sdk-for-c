// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_str_private.h"
#include <az_span_builder_internal.h>
#include <az_span_malloc_internal.h>
#include <az_span_writer_internal.h>

#include <_az_cfg.h>

// az_span_writer_size() and its utilities

AZ_NODISCARD az_result az_span_add_size(int32_t size, az_span span, int32_t * out) {
  // TODO: update
  *out = size + az_span_length(span);
  return AZ_OK;
}

AZ_ACTION_FUNC(az_span_add_size, int32_t, az_span_action)

AZ_NODISCARD az_result az_span_writer_size(az_span_writer const writer, int32_t * out_size) {
  AZ_CONTRACT_ARG_NOT_NULL(out_size);

  *out_size = 0;
  return az_span_writer_do(writer, az_span_add_size_action(out_size));
}

// az_span_writer_as_dynamic_str_writer() and its utilities

AZ_NODISCARD az_result az_span_writer_to_static_str_writer(
    az_span_writer const span_writer,
    az_span span,
    char const ** const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  AZ_RETURN_IF_FAILED(az_span_writer_do(span_writer, az_span_append_action(&span)));
  AZ_RETURN_IF_FAILED(az_span_append(span, AZ_SPAN_ZEROBYTE, &span));
  *out = (char const *)az_span_ptr(span);
  return AZ_OK;
}

AZ_NODISCARD az_result az_dynamic_span(size_t const size, az_span_action const mut_span_action) {
  az_span span;
  AZ_RETURN_IF_FAILED(az_span_malloc(size, &span));
  az_result const result = az_span_action_do(mut_span_action, span);
  az_span_free(&span);
  return result;
}

typedef struct {
  az_span_writer span_writer;
  az_str_action write_str;
} az_str_action_to_mut_span_action_data;

AZ_ACTION_FUNC(
    az_str_action_to_mut_span_action,
    az_str_action_to_mut_span_action_data,
    az_span_action)

AZ_NODISCARD az_result az_str_action_to_mut_span_action(
    az_str_action_to_mut_span_action_data self,
    az_span span,
    az_str_action_to_mut_span_action_data * out) {
  (void)self;

  char const * str = NULL;
  AZ_RETURN_IF_FAILED(az_span_writer_to_static_str_writer(out->span_writer, span, &str));
  AZ_RETURN_IF_FAILED(az_str_action_do(out->write_str, str));
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_writer_as_dynamic_str_writer(
    az_span_writer const span_writer,
    az_str_action const write_str) {
  int32_t size = 0;
  AZ_RETURN_IF_FAILED(az_span_writer_size(span_writer, &size));
  {
    az_str_action_to_mut_span_action_data data = {
      .span_writer = span_writer,
      .write_str = write_str,
    };
    AZ_RETURN_IF_FAILED(az_dynamic_span(size, az_str_action_to_mut_span_action_action(&data)));
  }
  return AZ_OK;
}
