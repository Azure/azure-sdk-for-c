// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_INTERNAL_H
#define _az_SPAN_INTERNAL_H

#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_span _az_span_set_length(az_span source, int32_t new_length)
{
  return az_span_init(az_span_ptr(source), new_length, az_span_capacity(source));
}

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_INTERNAL_H
