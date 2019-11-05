// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_H
#define AZ_KEYVAULT_H

#include <_az_cfg_prefix.h>

typedef struct {
  az_span uri;
  //TODO: Azure Credentials
} az_keyvault_keys_client;

typedef struct {
  //API version
  //Pipeline configuration options
} az_keyvault_keys_client_options;

typedef struct {
  //key size(size_t, typical default is 4096) 
  //Expires date 
  //All the REST options
} az_keyvault_keys_keys_options;

#include <_az_cfg_suffix.h>

#endif
