// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <_az_cfg.h>

// Note: Options can be NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * client,
    az_span uri,
    /*Azure Credentials */
    az_keyvault_keys_client_options const * options) {
  (void)client;
  (void)uri;
  (void)options;

  return AZ_OK;
}

// TODO #define AZ_KEYVAULT_KEYS_KEYTYPE_XXXX
// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_createKey(
    az_keyvault_keys_client * client,
    az_span keyname,
    az_span keytype,
    az_keyvault_keys_keys_options * options,
    az_span * out) {
  (void)client;
  (void)keyname;
  (void)keytype;
  (void)options;
  (void)out;
  return AZ_OK;
}
