// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_WRITER_H
#define AZ_SPAN_WRITER_H

#include <az_str.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_ACTION_TYPE(az_span_writer, az_span_action)

/**
 * A span of spans of bytes.
 */
typedef struct {
  az_span const * begin;
  size_t size;
} az_span_span;

/**
 * Emits all spans from @self into @action.
 */
AZ_NODISCARD az_result
az_span_span_as_writer(az_span_span const * const self, az_span_action const write_span);

/**
 * @az_span_span_emit as an action of type @az_span_emitter.
 */
AZ_ACTION_FUNC(az_span_span_as_writer, az_span_span const, az_span_writer)

/**
 * Calculates a size of a contigous buffer to store all spans from @self.
 */
AZ_NODISCARD az_result az_span_writer_size(az_span_writer const self, size_t * const out_size);

/**
 * The function creates a temporary zero-terminated string from @span_writer in dynamic memory.
 * The string is passed to the given @write_str. After @write_str is returned, the temporary 
 * string is destroyed.
 */
AZ_NODISCARD az_result
az_span_writer_as_dynamic_str_writer(az_span_writer const span_writer, az_str_action const write_str);

#include <_az_cfg_suffix.h>

#endif
