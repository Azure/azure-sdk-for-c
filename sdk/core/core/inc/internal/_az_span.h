// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _AZ_SPAN_H
#define _AZ_SPAN_H

#include <az_span_def.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span const span) {
  return span.size == 0 || (span.begin != NULL && span.begin <= span.begin + span.size - 1);
}

#include <_az_cfg_suffix.h>

#endif
