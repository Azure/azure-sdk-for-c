// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_SEQ_H
#define AZ_SPAN_SEQ_H

#include <az_str.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_CALLBACK_TYPE(az_span_seq, az_span_append)

/**
 * A span of spans of bytes.
 */
typedef struct {
  az_const_span const * begin;
  size_t size;
} az_span_span;

/**
 * Emits all spans from `*p_span_span` into @append.
 */
AZ_NODISCARD az_result
az_span_span_to_seq(az_span_span const * const p_span_span, az_span_append const append);

/**
 * @az_span_span_to_seq as a callback.
 */
AZ_CALLBACK_FUNC(az_span_span_to_seq, az_span_span const *, az_span_seq)

/**
 * Calculates a size of a contigous buffer to store all spans from @seq.
 */
AZ_NODISCARD az_result az_span_seq_size(az_span_seq const seq, size_t * const out_size);

/**
 * The function creates a temporary zero-terminated string from @seq in dynamic memory.
 * The string is passed to the given @callback. After @callback is returned, the temporary string is
 * destroyed.
 */
AZ_NODISCARD az_result
az_span_seq_to_tmp_str(az_span_seq const seq, az_str_callback const str_callback);

#include <_az_cfg_suffix.h>

#endif
