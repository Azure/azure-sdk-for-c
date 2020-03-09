// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_optional_types.h
 *
 * @brief Types that may not have a value.
 */

#ifndef _az_OPTIONAL_TYPES_H
#define _az_OPTIONAL_TYPES_H

#include <stdbool.h>

#include <_az_cfg_prefix.h>

/// Optional boolean type.
typedef struct
{
  /// Indicates whether \a data has a value.
  bool is_present : 1;

  /// Value.
  bool data : 1;
} az_optional_bool;

/// Creates an optional bool type with the provided value.
///
/// @param value A boolean value for the result to have.
///
/// @return An optional bool type having a \a value.
AZ_INLINE AZ_NODISCARD az_optional_bool az_optional_bool_create(bool value)
{
  return (az_optional_bool){
    .is_present = true,
    .data = value,
  };
}

/// Creates an optional bool type with no data.
///
/// @return An optional bool type having no data.
AZ_NODISCARD AZ_INLINE az_optional_bool az_optional_bool_create_no_data()
{
  return (az_optional_bool){ .is_present = false };
}

#include <_az_cfg_suffix.h>

#endif // _az_OPTIONAL_TYPES_H
