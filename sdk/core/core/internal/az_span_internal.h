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

/**
 * @brief Set all the content of the span to @b fill without updating the length of the span.
 * This is util to set memory to zero before using span or make sure span is clean before use
 *
 * @param span source span
 * @param fill byte to use for filling span
 */
AZ_INLINE void az_span_fill(az_span span, uint8_t fill) {
  memset(az_span_ptr(span), fill, az_span_capacity(span));
}

#include <_az_cfg_suffix.h>

#endif
