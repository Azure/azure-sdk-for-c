// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "../inc/internal/_az_mut_span.h"
#include "../inc/internal/_az_span.h"
#include "../inc/internal/az_contract.h"
#include <az_mut_span.h>

#include <ctype.h>

#include <_az_cfg.h>

#define AZ_CONTRACT_ARG_VALID_MUT_SPAN(span) AZ_CONTRACT(az_mut_span_is_valid(span), AZ_ERROR_ARG)
#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

AZ_NODISCARD az_result
az_mut_span_move(az_mut_span const buffer, az_span const src, az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  AZ_CONTRACT(buffer.size >= src.size, AZ_ERROR_BUFFER_OVERFLOW);

  if (!az_span_is_empty(src)) {
    memmove((void *)buffer.begin, (void const *)src.begin, src.size);
  }

  out_result->begin = buffer.begin;
  out_result->size = src.size;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_mut_span_to_str(az_mut_span const buffer, az_span const src, az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (buffer.size < src.size + 1) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_span_is_empty(src)) {
    az_mut_span result = { 0 };
    AZ_RETURN_IF_FAILED(az_mut_span_move(buffer, src, &result));
  }

  buffer.begin[src.size] = '\0';

  out_result->begin = buffer.begin;
  out_result->size = src.size + 1;

  return AZ_OK;
}
