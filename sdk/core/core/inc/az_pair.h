// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_PAIR_H
#define AZ_PAIR_H

#include <az_action.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg_prefix.h>

/// A pair of strings.
typedef struct {
  az_span key;
  az_span value;
} az_pair;

/// A span of pairs.
typedef struct {
  az_pair const * begin;
  size_t size;
} az_pair_span;

typedef struct {
  az_pair * begin;
  size_t size;
} az_mut_pair_span;

typedef struct {
  az_mut_pair_span buffer;
  size_t size;
} az_pair_span_builder;

/**
 * Creates a span of pair span builder.
 *
 * @param buffer a buffer for writing.
 */
AZ_NODISCARD AZ_INLINE az_pair_span_builder
az_pair_span_builder_create(az_mut_pair_span const buffer) {
  return (az_pair_span_builder){
    .buffer = buffer,
    .size = 0,
  };
}

/**
 * Append a given pair.
 */
AZ_NODISCARD az_result
az_pair_span_builder_append(az_pair_span_builder * const self, az_pair const pair);

/// @az_pair_action is a callback with one argument @az_pair.
AZ_ACTION_TYPE(az_pair_action, az_pair)

/// @var az_pair_writer writes @var az_pair sequences.
AZ_ACTION_TYPE(az_pair_writer, az_pair_action)

AZ_NODISCARD az_result
az_pair_span_as_writer(az_pair_span const * const self, az_pair_action const write_pair);

/**
 * Creates @var az_pair_span_writer from @az_write_pair_span.
 */
AZ_ACTION_FUNC(az_pair_span_as_writer, az_pair_span const, az_pair_writer)

AZ_NODISCARD AZ_INLINE bool az_pair_span_eq(az_pair const a, az_pair const b) {
  return az_span_is_equal(a.key, b.key) && az_span_is_equal(a.value, b.value);
}

#include <_az_cfg_suffix.h>

#endif
