// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_SEQ_H
#define AZ_SPAN_SEQ_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_CALLBACK_DECL(az_span_seq, az_span_visitor)

typedef struct {
  az_const_span const * begin;
  size_t size;
} az_span_span;

az_span_seq az_span_span_to_seq(az_span_span const * const p_span);

az_result az_span_seq_size(az_span_seq const seq, size_t * const out_size);

#include <_az_cfg_suffix.h>

#endif
