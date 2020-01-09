// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_MALLOC_H
#define _az_SPAN_MALLOC_H

#include <az_mut_span.h>
#include <az_result.h>

#include <_az_cfg_extern_include_prefix.h>

#include <stddef.h>

#include <_az_cfg_extern_include_suffix.h>

#include <_az_cfg_prefix.h>

/**
 * Allocates a span on a heap.
 */
AZ_NODISCARD az_result az_span_malloc(size_t const size, az_mut_span * const out);

/**
 * Frees the given span.
 *
 * Precondition: the given span should be previously allocated by @var az_span_malloc function.
 */
void az_span_free(az_mut_span * const p);

#include <_az_cfg_suffix.h>

#endif
