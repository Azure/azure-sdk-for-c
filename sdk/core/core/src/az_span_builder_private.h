// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_BUILDER_PRIVATE_H
#define _az_SPAN_BUILDER_PRIVATE_H

#include <az_span_builder.h>

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * Append a single byte.
 */
AZ_NODISCARD az_result az_span_builder_append_byte(az_span_builder * const self, uint8_t const c);

/**
 * Append zeros.
 */
AZ_NODISCARD az_result
az_span_builder_append_zeros(az_span_builder * const self, size_t const size);

#include <_az_cfg_suffix.h>

#endif
