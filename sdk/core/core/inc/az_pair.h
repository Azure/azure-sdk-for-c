// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PAIR_H
#define _az_PAIR_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg_prefix.h>

/**
 * @brief a pair of az_span of bytes as a key and value
 *
 */
typedef struct {
  az_span key;
  az_span value;
} az_pair;

AZ_NODISCARD AZ_INLINE az_pair az_pair_init(az_span key, az_span value) {
  return (az_pair){ .key = key, .value = value };
}

AZ_NODISCARD AZ_INLINE az_pair az_pair_null() {
  return az_pair_init(az_span_null(), az_span_null());
}

AZ_NODISCARD AZ_INLINE az_pair az_pair_from_str(char * key, char * value) {
  return az_pair_init(az_span_from_str(key), az_span_from_str(value));
}

#include <_az_cfg_suffix.h>

#endif
