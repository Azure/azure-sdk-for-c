// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_WRITER_INTERNAL_H
#define _az_SPAN_WRITER_INTERNAL_H

#include <az_action.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_action.h>
#include <az_span_internal.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

AZ_ACTION_TYPE(az_span_writer, az_span_action)

/**
 * A callback that accepts zero-terminated string `char const *`.
 */
AZ_ACTION_TYPE(az_str_action, char const *)

/**
 * Calculates a size of a contiguous buffer to store all spans from @var self.
 */
AZ_NODISCARD az_result az_span_writer_size(az_span_writer const self, int32_t * out_size);

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
