// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_PIPELINE_H
#define AZ_HTTP_PIPELINE_H

#include <az_http_policy.h>
#include <az_http_request_builder.h>
#include <az_mut_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_http_policy uniquerequestid;
  az_http_policy retry;
  az_http_policy authentication;
  az_http_policy logging;
  az_http_policy bufferresponse;
  az_http_policy distributedtracing;
  az_http_policy transport;
} az_http_policies;

AZ_NODISCARD az_result az_http_policies_init(az_http_policies * const self);

// Start the pipeline
AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_request_builder * const hrb,
    az_mut_span const * const response,
    az_http_policies const * const policies);

#include <_az_cfg_suffix.h>
#endif
