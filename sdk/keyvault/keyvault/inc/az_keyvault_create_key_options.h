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

#include <_az_cfg_suffix.h>

#endif
