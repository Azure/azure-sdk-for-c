// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_INTERNAL_SPAN_INTERNAL_H
#define _az_INTERNAL_SPAN_INTERNAL_H

#include <az_action.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span const span) {
  return span.size == 0 || (span.begin != NULL && span.begin <= span.begin + span.size - 1);
}

#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

#include <_az_cfg_suffix.h>

#endif
