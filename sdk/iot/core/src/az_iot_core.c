// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_core.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

AZ_NODISCARD az_span az_span_token(az_span source, az_span delimiter, az_span* out_remainder)
{
  AZ_PRECONDITION_VALID_SPAN(delimiter, 1, false);
  AZ_PRECONDITION_NOT_NULL(out_remainder);

  if (az_span_length(source) == 0)
  {
    return AZ_SPAN_NULL;
  }
  else
  {
    int32_t index = az_span_find(source, delimiter);

    if (index != -1)
    {
      *out_remainder = az_span_slice(source, index + az_span_length(delimiter), az_span_length(source));

      return az_span_slice(source, 0, index);
    }
    else
    {
      *out_remainder = AZ_SPAN_NULL;

      return source;
    } 
  }
}
