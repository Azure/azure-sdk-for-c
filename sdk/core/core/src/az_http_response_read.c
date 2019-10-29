// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_read.h>

#include <az_http_result.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_response_state_init(az_span const buffer, az_http_response_state * const out) {
  AZ_CONTRACT_ARG_VALID_SPAN(buffer);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  *out = (az_http_response_state){ .reader = az_span_reader_create(buffer) };
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_state_read_status() { return AZ_ERROR_NOT_IMPLEMENTED; }

AZ_NODISCARD az_result az_http_response_state_read_header() { return AZ_ERROR_NOT_IMPLEMENTED; }

AZ_NODISCARD az_result az_http_response_state_read_body() { return AZ_ERROR_NOT_IMPLEMENTED; }

AZ_NODISCARD az_result az_http_response_state_read(
    az_http_response_state * const self,
    az_http_response_value * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  switch (self->kind) {
    case AZ_HTTP_RESPONSE_STATE_STATUS:
      return az_http_response_state_read_status();
    case AZ_HTTP_RESPONSE_STATE_HEADER:
      return az_http_response_state_read_header();
    case AZ_HTTP_RESPONSE_STATE_BODY:
      return az_http_response_state_read_body();
    default:
      return AZ_HTTP_ERROR_INVALID_STATE;
  }
}
