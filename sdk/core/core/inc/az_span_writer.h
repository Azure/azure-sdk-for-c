// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_WRITER_H
#define AZ_SPAN_WRITER_H

#include <az_str.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_ACTION_TYPE(az_span_writer, az_write_span)

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
az_span_span_as_writer(az_span_span const * const self, az_write_span const write_span);

/**
 * @az_span_span_emit as an action of type @az_span_emitter.
 */
AZ_ACTION_FUNC(az_span_span_as_writer, az_span_span const, az_span_writer)

/**
 * Calculates a size of a contigous buffer to store all spans from @self.
 */
AZ_NODISCARD az_result az_span_writer_size(az_span_writer const self, size_t * const out_size);

/**
 * The function creates a temporary zero-terminated string from @emmiter in dynamic memory.
 * The string is passed to the given @str_action. 
 * 
 * Note: After @str_action is returned, the temporary string is destroyed.
 */
AZ_NODISCARD az_result
az_span_emitter_to_tmp_str(az_span_writer const writer, az_write_str const write_str);

#include <_az_cfg_suffix.h>

#endif
