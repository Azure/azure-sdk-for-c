// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_INTERNAL_H
#define _az_SPAN_INTERNAL_H

#include <az_precondition_internal.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

// Use this helper to figure out how much the sliced_span has moved in comparison to the
// original_span while writing and slicing a copy of the original.
// The \p sliced_span must be some slice of the \p original_span (and have the same backing memory).
AZ_INLINE AZ_NODISCARD int32_t _az_span_diff(az_span sliced_span, az_span original_span)
{
  int32_t answer = az_span_size(original_span) - az_span_size(sliced_span);

  // The passed in span parameters cannot be any two arbitrary spans.
  // This validates the span parameters are valid and one is a sub-slice of another.
  _az_PRECONDITION(answer == (int32_t)(az_span_ptr(sliced_span) - az_span_ptr(original_span)));
  return answer;
}

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_INTERNAL_H
