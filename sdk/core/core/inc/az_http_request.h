// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_contract.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

typedef struct {
  void const * begin;
  void const * end;
} az_iter_data;

typedef struct az_span_iter az_span_iter;

typedef az_result (*az_span_iter_func)(az_span_iter * const p_i, az_span * const out);

struct az_span_iter {
  az_span_iter_func func;
  az_iter_data data;
};

static inline az_result az_span_iter_call(az_span_iter * const p_i, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);
  AZ_CONTRACT_ARG_NOT_NULL(p_i->func);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return p_i->func(p_i, out);
}

// pair

typedef struct {
  az_const_span key;
  az_const_span value;
} az_pair;

// pair iterator

typedef struct az_pair_iter az_pair_iter;

typedef az_result (*az_pair_iter_func)(az_pair_iter * const p_i, az_pair * const out);

struct az_pair_iter {
  az_pair_iter_func func;
  az_iter_data data;
};

static inline az_result az_pair_iter_call(az_pair_iter * const p_i, az_pair * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);
  AZ_CONTRACT_ARG_NOT_NULL(p_i->func);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return p_i->func(p_i, out);
}

typedef struct {
  az_pair const * begin;
  size_t size;
} az_pair_span;

az_pair_iter az_pair_span_to_iter(az_pair_span const span);

// request

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_iter query;
  az_pair_iter headers;
  az_const_span body;
} az_http_request;

az_result az_http_request_to_buffer(
    az_http_request * const p_request,
    az_span const span,
    az_span * const out);

#include <_az_cfg_suffix.h>

#endif
