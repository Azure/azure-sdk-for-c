// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_INTERNAL_H
#define _az_SPAN_INTERNAL_H

#include <az_contract.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span const span) {
  return az_span_length_get(span) == 0
      || (az_span_ptr_get(span) != NULL
          && az_span_ptr_get(span) <= az_span_ptr_get(span) + az_span_length_get(span) - 1);
}

#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

#include <_az_cfg_suffix.h>

#endif
