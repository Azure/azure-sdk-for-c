// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_CREATE_KEY_OPTIONS_H
#define AZ_KEYVAULT_CREATE_KEY_OPTIONS_H

#include <az_optional_bool.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_optional_bool enabled;
  /* TODO: adding next options
  Datetime not_before;
  Datetime expires_on
  List tags
  List keyOperations
  */
} az_keyvault_create_key_options;

/**
 * @brief init an az_keyvault_create_key_options
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_keyvault_create_key_options_init(az_keyvault_create_key_options * const self) {
  *self = (az_keyvault_create_key_options){ 0 };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
