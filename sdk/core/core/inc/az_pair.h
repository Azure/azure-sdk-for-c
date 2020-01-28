// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PAIR_H
#define _az_PAIR_H

#include <az_action.h>
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

/**
 * @brief an immutable span of az_pairs where begin points to the first az_pair
 * and size represents the total number of az_pairs
 */
typedef struct {
  az_pair const * begin;
  size_t size;
} az_pair_span;

AZ_NODISCARD AZ_INLINE az_pair az_pair_init(az_span key, az_span value) {
  return (az_pair){ .key = key, .value = value };
}

AZ_NODISCARD AZ_INLINE az_pair az_pair_null() {
  return az_pair_init(az_span_null(), az_span_null());
}

AZ_NODISCARD AZ_INLINE az_pair az_pair_from_str(char * key, char * value) {
  return az_pair_init(az_span_from_str(key), az_span_from_str(value));
}

/// @az_pair_action is a callback with one argument @az_pair.
AZ_ACTION_TYPE(az_pair_action, az_pair)

/// @var az_pair_writer writes @var az_pair sequences.
AZ_ACTION_TYPE(az_pair_writer, az_pair_action)

AZ_NODISCARD az_result az_pair_as_writer(az_pair self, az_pair_action write_pair, az_pair * out);

/**
 * Creates @var az_pair_span_writer from @az_write_pair_span.
 */
AZ_ACTION_FUNC(az_pair_as_writer, az_pair, az_pair_writer)

#include <_az_cfg_suffix.h>

#endif
