// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_JSON_WEB_KEY_TYPE_H
#define AZ_KEYVAULT_JSON_WEB_KEY_TYPE_H

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_NONE = 0,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_EC = 1,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_EC_HSM = 2,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA = 3,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA_HSM = 4,
  AZ_KEYVAULT_JSON_WEB_KEY_TYPE_OCT = 5,
} az_keyvault_json_web_key_type;

#include <_az_cfg_suffix.h>

#endif
