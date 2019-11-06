// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_H
#define AZ_KEYVAULT_H

#include <az_contract.h>
#include <az_result.h>
#include <az_span.h>

#include <stdlib.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span uri;
  // TODO: Azure Credentials
} az_keyvault_keys_client;

typedef struct {
  // API version
  // Pipeline configuration options
  az_span version;
} az_keyvault_keys_client_options;

typedef struct {
  // key size(size_t, typical default is 4096)
  // Expires date
  // All the REST options
  az_span option;
} az_keyvault_keys_keys_options;

AZ_NODISCARD az_result az_keyvault_keys_client_init(
    az_keyvault_keys_client * client,
    az_span uri,
    /*Azure Credentials */
    az_keyvault_keys_client_options * options);

AZ_NODISCARD az_result az_keyvault_keys_createKey(
    az_keyvault_keys_client * client,
    az_span keyname,
    az_span keytype,
    az_keyvault_keys_keys_options * options,
    az_span * out);

#include <_az_cfg_suffix.h>

#endif
