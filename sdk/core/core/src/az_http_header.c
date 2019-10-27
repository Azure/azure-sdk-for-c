// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_header.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_header_emit_spans(az_pair const * p_header, az_span_append const append) {
  AZ_CONTRACT_ARG_NOT_NULL(p_header);

  AZ_RETURN_IF_FAILED(az_span_append_do(append, p_header->key));
  AZ_RETURN_IF_FAILED(az_span_append_do(append, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(az_span_append_do(append, p_header->value));
  return AZ_OK;
}
