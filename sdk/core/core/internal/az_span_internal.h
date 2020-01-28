// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_INTERNAL_H
#define _az_SPAN_INTERNAL_H

#include <az_contract_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span const span) {
  return az_span_length(span) == 0
      || (az_span_ptr(span) != NULL
          && az_span_ptr(span) <= az_span_ptr(span) + az_span_length(span) - 1);
}

#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

AZ_NODISCARD AZ_INLINE bool az_span_is_empty(az_span const span) {
  return az_span_length(span) == 0;
}

/**
 * @brief Set all the content of the span to @b fill without updating the length of the span.
 * This is util to set memory to zero before using span or make sure span is clean before use
 *
 * @param span source span
 * @param fill byte to use for filling span
 * @return AZ_INLINE az_span_fill
 */
AZ_INLINE void az_span_fill(az_span span, uint8_t fill) {
  memset(az_span_ptr(span), fill, az_span_capacity(span));
}

#include <_az_cfg_suffix.h>

#endif
