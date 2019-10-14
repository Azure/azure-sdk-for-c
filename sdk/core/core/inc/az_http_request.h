// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_contract.h>
#include <az_iter_data.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

typedef az_result (*az_pair_event_func)(void * context, az_pair const pair);

typedef struct {
  az_pair_event_func func;
  void * context;
} az_pair_event;

AZ_INLINE az_result az_pair_event_call(az_pair_event const event, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(event.func);

  return event.func(event.context, pair);
}

typedef az_result (*az_pair_event_list_func)(void * context, az_pair_event const event);

typedef struct {
  az_pair_event_list_func func;
  void * context;
} az_pair_event_list;

AZ_INLINE az_result az_pair_event_list_call(az_pair_event_list const list, az_pair_event const event) {
  AZ_CONTRACT_ARG_NOT_NULL(list.func);

  return list.func(list.context, event);
}

// request

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_event_list query;
  az_pair_event_list headers;
  az_const_span body;
} az_http_request_e;

az_result az_http_request_e_to_buffer(
    az_http_request_e const * const p_request,
    az_span const span,
    az_span * const out);

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_iter query;
  az_pair_iter headers;
  az_const_span body;
} az_http_request;

az_result az_http_request_to_buffer(
    az_http_request const * const p_request,
    az_span const span,
    az_span * const out);

typedef struct {
  az_pair_iter original_headers;
} az_http_standard_headers_data;

/**
 * Note: `*p_request` should not be used after `*out` is destroyed.
 */
az_result az_http_standard_headers_policy(
    az_http_request * const p_request,
    az_http_standard_headers_data * const out);

#include <_az_cfg_suffix.h>

#endif
