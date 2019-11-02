// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_MALLOC_H
#define AZ_SPAN_MALLOC_H

#include <az_mut_span.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result az_span_malloc(size_t const size, az_mut_span * const out);

void az_span_free(az_mut_span * const p);

#include <_az_cfg_suffix.h>

#endif
