// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_optional_types.h
 *
 * @brief definition for and utilities to use types as an optional type
 */

#ifndef _az_OPTIONAL_TYPES_H
#define _az_OPTIONAL_TYPES_H

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef struct {
  bool is_present : 1;
  bool data : 1;
} az_optional_bool;

AZ_NODISCARD AZ_INLINE az_optional_bool az_optional_bool_create(bool value) {
  return (az_optional_bool){
    .is_present = true,
    .data = value,
  };
}

AZ_NODISCARD AZ_INLINE az_optional_bool az_optional_bool_create_no_data() {
  return (az_optional_bool){ .is_present = false };
}

#include <_az_cfg_suffix.h>

#endif
