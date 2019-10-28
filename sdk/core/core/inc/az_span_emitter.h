// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_EMITTER_H
#define AZ_SPAN_EMITTER_H

#include <az_str.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_ACTION_TYPE(az_span_emitter, az_span_action)

/**
 * A span of spans of bytes.
 */
AZ_SPAN_TYPE(az_span_span, az_span const)

/**
 * Emits all spans from @self into @append.
 */
AZ_NODISCARD az_result
az_span_span_emit(az_span_span const * const self, az_span_action const action);

/**
 * @az_span_span_emit as a callback.
 */
AZ_ACTION_FUNC(az_span_span_emit, az_span_span const, az_span_emitter)

/**
 * Calculates a size of a contigous buffer to store all spans from @self.
 */
AZ_NODISCARD az_result az_span_emitter_size(az_span_emitter const self, size_t * const out_size);

/**
 * The function creates a temporary zero-terminated string from @emmiter in dynamic memory.
 * The string is passed to the given @str_action. 
 * 
 * Note: After @str_action is returned, the temporary string is destroyed.
 */
AZ_NODISCARD az_result
az_span_emitter_to_tmp_str(az_span_emitter const emitter, az_str_action const str_action);

#include <_az_cfg_suffix.h>

#endif
