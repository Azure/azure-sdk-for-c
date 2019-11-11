// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_url.h>

#include <az_http_query.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_url_emit_span_seq(az_url const * const self, az_span_action const write_span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, self->host));
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, self->path));
  AZ_RETURN_IF_FAILED(az_http_query_as_span_writer(self->query, write_span));
  return AZ_OK;
}
