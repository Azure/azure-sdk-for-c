// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_read.h>

#include <az_http_result.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_http_response_value
az_http_response_value_create_status(az_http_response_status const status) {
  return (az_http_response_value){ .kind = AZ_HTTP_RESPONSE_VALUE_STATUS, .data.status = status };
}

AZ_NODISCARD az_http_response_state az_http_response_state_create(az_span const buffer) {
  return (az_http_response_state){ .reader = az_span_reader_create(buffer) };
}

/**
 * Status line https://tools.ietf.org/html/rfc7230#section-3.1.2
 */
AZ_NODISCARD az_result az_http_response_state_read_status(
    az_http_response_state * const self,
    az_http_response_status * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
  // https://tools.ietf.org/html/rfc7230#section-2.6


  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_http_response_state_read_header() { return AZ_ERROR_NOT_IMPLEMENTED; }

AZ_NODISCARD az_result az_http_response_state_read_body() { return AZ_ERROR_NOT_IMPLEMENTED; }

AZ_NODISCARD az_result az_http_response_state_read(
    az_http_response_state * const self,
    az_http_response_value * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  switch (self->kind) {
    case AZ_HTTP_RESPONSE_STATE_STATUS: {
      az_http_response_status status;
      AZ_RETURN_IF_FAILED(az_http_response_state_read_status(self, &status));
      *out = az_http_response_value_create_status(status);
      return AZ_OK;
    }
    case AZ_HTTP_RESPONSE_STATE_HEADER:
      return az_http_response_state_read_header();
    case AZ_HTTP_RESPONSE_STATE_BODY:
      return az_http_response_state_read_body();
    default:
      return AZ_HTTP_ERROR_INVALID_STATE;
  }
}
