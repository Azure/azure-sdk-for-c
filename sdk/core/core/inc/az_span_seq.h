// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_SEQ_H
#define AZ_SPAN_SEQ_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_CALLBACK_TYPE(az_span_seq, az_span_visitor)

typedef struct {
  az_const_span const * begin;
  size_t size;
} az_span_span;

az_result az_span_span_to_seq(az_span_span const * const context, az_span_visitor const visitor);

AZ_CALLBACK_FUNC(az_span_span_to_seq, az_span_span const *, az_span_seq)

az_result az_span_add_size(size_t * const p_size, az_const_span const span);

AZ_CALLBACK_FUNC(az_span_add_size, size_t *, az_span_visitor)

az_result az_span_seq_size(az_span_seq const seq, size_t * const out_size);

az_result az_span_seq_to_new_str(az_span_seq const seq, char ** const out);

#include <_az_cfg_suffix.h>

#endif
