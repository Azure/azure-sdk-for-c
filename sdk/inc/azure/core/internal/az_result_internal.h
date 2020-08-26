// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_RESULT_INTERNAL_H
#define _az_RESULT_INTERNAL_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

#define _az_RETURN_IF_FAILED(exp) \
  do \
  { \
    az_result const _az_res = (exp); \
    if (az_failed(_az_res)) \
    { \
      return _az_res; \
    } \
  } while (0)

#define _az_RETURN_IF_NOT_ENOUGH_SIZE(span, required_size) \
  do \
  { \
    int32_t const _az_req_sz = (required_size); \
    if (az_span_size(span) < _az_req_sz || _az_req_sz < 0) \
    { \
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE; \
    } \
  } while (0)

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_RESULT_INTERNAL_H
