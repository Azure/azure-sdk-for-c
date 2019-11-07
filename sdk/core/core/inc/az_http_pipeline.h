// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_PIPELINE_H
#define AZ_HTTP_PIPELINE_H

#include <az_contract.h>
#include <az_http_policy.h>
#include <az_http_request.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

// Start the pipeline
AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

#include <_az_cfg_suffix.h>
#endif
