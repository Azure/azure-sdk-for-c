// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_header.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_header_emit_span_seq(az_pair const * const self, az_write_span const write_span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_write_span_do(write_span, self->key));
  // TODO: it looks like we may no need space. 
  // Probably ":" would be sufficient.
  // We need to check RFC and CURL SLIST.
  AZ_RETURN_IF_FAILED(az_write_span_do(write_span, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(az_write_span_do(write_span, self->value));
  return AZ_OK;
}
