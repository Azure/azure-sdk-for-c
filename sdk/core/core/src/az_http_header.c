// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_header.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_header_emit_span_seq(az_pair const * const self, az_span_action const action) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_action_do(action, self->key));
  // TODO: it looks like we may no need space. 
  // Probably ":" would be sufficient.
  // We need to check RFC and CURL SLIST.
  AZ_RETURN_IF_FAILED(az_span_action_do(action, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(az_span_action_do(action, self->value));
  return AZ_OK;
}
