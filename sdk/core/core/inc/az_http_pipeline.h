// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_PIPELINE_H
#define AZ_HTTP_PIPELINE_H

#include <az_http_request_builder.h>
#include <az_mut_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_http_request_builder builder;
  az_mut_span response;
} az_http_policy_arg;

// Start the pipeline
AZ_NODISCARD az_result az_http_pipeline_process(az_http_policy_arg * const arg);

#include <_az_cfg_suffix.h>
#endif
