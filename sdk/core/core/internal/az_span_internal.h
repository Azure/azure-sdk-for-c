// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_INTERNAL_H
#define _az_SPAN_INTERNAL_H

#include <az_span.h>
#include <az_precondition_internal.h>

#include <_az_cfg_prefix.h>

AZ_INLINE AZ_NODISCARD int32_t _az_span_diff(az_span new_span, az_span old_span)
{
  int32_t answer = az_span_size(old_span) - az_span_size(new_span);
  AZ_PRECONDITION(answer == (int32_t)(az_span_ptr(new_span) - az_span_ptr(old_span)));
  return answer;
}

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_INTERNAL_H
