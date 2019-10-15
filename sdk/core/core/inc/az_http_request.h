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

typedef ptrdiff_t az_callback_data;

#define AZ_CAT(A, B) A##B

#define AZ_CALLBACK_ARG(NAME) AZ_CAT(NAME, _arg)

#define AZ_CALLBACK_DECL(NAME, ARG) \
  typedef ARG AZ_CALLBACK_ARG(NAME); \
  typedef struct { \
    az_result (*func)(az_callback_data const, ARG const); \
    az_callback_data data; \
  } NAME;

#define AZ_CALLBACK_DATA(NAME, DATA, CALLBACK) \
  AZ_STATIC_ASSERT(sizeof(DATA) <= sizeof(az_callback_data)) \
  AZ_INLINE CALLBACK NAME(DATA const data, az_result (* const func)(DATA const, AZ_CALLBACK_ARG(CALLBACK) const)) { \
    return (CALLBACK){ \
      .func = (az_result (*)(az_callback_data, AZ_CALLBACK_ARG(CALLBACK)))func, \
      .data = (az_callback_data)data, \
    }; \
  }

AZ_CALLBACK_DECL(az_pair_visitor, az_pair);

AZ_CALLBACK_DECL(az_pair_seq, az_pair_visitor);

az_pair_seq az_pair_span_to_seq(az_pair_span const * const p_span);

// request

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_seq query;
  az_pair_seq headers;
  az_const_span body;
} az_http_request;

az_result az_http_request_to_buffer(
    az_http_request const * const p_request,
    az_span const span,
    az_span * const out);

typedef struct {
  az_pair_seq headers;
} az_http_standard_policy;

/**
 * Note: `*p_request` should not be used after `*out` is destroyed.
 */
az_result az_http_standard_policy_create(
    az_http_request * const p_request,
    az_http_standard_policy * const out);

#include <_az_cfg_suffix.h>

#endif
