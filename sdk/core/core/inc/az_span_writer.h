// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_WRITER_H
#define _az_SPAN_WRITER_H

#include <az_action.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_span.h>
#include <az_str.h>

#include <_az_cfg_extern_include_prefix.h>

#include <stddef.h>

#include <_az_cfg_extern_include_suffix.h>

#include <_az_cfg_prefix.h>

AZ_ACTION_TYPE(az_span_writer, az_span_action)

/**
 * Emits all spans from @var self into @var action.
 */
AZ_NODISCARD az_result
az_span_span_as_writer(az_span_span const * const self, az_span_action const write_span);

/**
 * @var az_span_span_emit as an action of type @var az_span_emitter.
 */
AZ_ACTION_FUNC(az_span_span_as_writer, az_span_span const, az_span_writer)

/**
 * Calculates a size of a contiguous buffer to store all spans from @var self.
 */
AZ_NODISCARD az_result az_span_writer_size(az_span_writer const self, size_t * const out_size);

/**
 * The function creates a temporary zero-terminated string from @var span_writer in dynamic memory.
 * The string is passed to the given @var write_str. After @var write_str is returned, the temporary
 * string is destroyed.
 */
AZ_NODISCARD az_result az_span_writer_as_dynamic_str_writer(
    az_span_writer const span_writer,
    az_str_action const write_str);

#include <_az_cfg_suffix.h>

#endif
