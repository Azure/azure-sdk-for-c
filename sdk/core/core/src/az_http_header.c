// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_header_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_header_as_span_writer(az_pair self, az_span_action write_span, az_pair * out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);
  (void)self;

  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, out->key));
  // TODO: it looks like we may no need space.
  // Probably ":" would be sufficient.
  // We need to check RFC and CURL SLIST.
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, AZ_SPAN_FROM_STR(": ")));
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, out->value));
  return AZ_OK;
}
