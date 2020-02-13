// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CONTRACT_INTERNAL_H
#define _az_CONTRACT_INTERNAL_H

#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT(condition, error) \
  do { \
    if (!(condition)) { \
      return error; \
    } \
  } while (0)

#define AZ_CONTRACT_ARG_NOT_NULL(arg) AZ_CONTRACT((arg) != NULL, AZ_ERROR_ARG)

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span span) {
  return az_span_length(span) == 0
      || (az_span_ptr(span) != NULL
          && az_span_ptr(span) <= az_span_ptr(span) + az_span_length(span) - 1);
}

#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

AZ_NODISCARD AZ_INLINE bool az_span_is_empty(az_span span) { return az_span_length(span) == 0; }

#include <_az_cfg_suffix.h>

#endif /* _az_CONTRACT_INTERNAL_H */
