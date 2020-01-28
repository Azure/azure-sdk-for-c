// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_MALLOC_INTERNAL_H
#define _az_SPAN_MALLOC_INTERNAL_H

#include <az_result.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

/**
 * Allocates a span on a heap.
 */
AZ_NODISCARD az_result az_span_malloc(size_t size, az_span * out);

/**
 * Frees the given span.
 *
 * Precondition: the given span should be previously allocated by @var az_span_malloc function.
 */
void az_span_free(az_span * p);

#include <_az_cfg_suffix.h>

#endif
