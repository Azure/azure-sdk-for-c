// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_action.h>

#include <az_span_reader.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_action_to_http_response_action(
    az_write_span const * const self,
    az_http_response_action * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_http_response_action_to_span_action(
    az_http_response_action const * const self,
    az_write_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return AZ_ERROR_NOT_IMPLEMENTED;
}
