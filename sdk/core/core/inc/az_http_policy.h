// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_POLICY_H
#define AZ_HTTP_POLICY_H

#include <az_contract.h>
#include <az_http_request.h>
#include <az_iter_data.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

// PipelinePolicies
//   Policies are non-allocating caveat the TransportPolicy
//   Transport policies can only allocate if the transport layer they call allocates
// Client ->
//  ===HttpPipelinePolicies===
//    UniqueRequestID
//    Retry
//    Authentication
//    Logging
//    Buffer Response
//    Distributed Tracing
//    TransportPolicy
//  ===Transport Layer===

typedef struct {
  az_span data;
} az_http_response_data;

az_result (*az_http_policy_process)(az_http_request * const p_request, az_http_response_data * const out);

typedef struct {
  az_span data;
} az_http_policy_data;

typedef struct {
  (*az_http_policy_process) process;
  az_http_policy_data data;
} az_http_policy;

typedef struct {
  az_http_policy * const p_policies;
} az_http_pipeline;

#include <_az_cfg_suffix.h>

#endif