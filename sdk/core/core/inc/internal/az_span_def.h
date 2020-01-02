// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_DEF_H
#define AZ_SPAN_DEF_H

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * An immutable span of bytes (octets).
 */
typedef struct {
  uint8_t const * begin;
  size_t size;
} az_span;

#include <_az_cfg_suffix.h>

#endif
