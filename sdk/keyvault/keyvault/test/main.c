// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <_az_cfg.h>

int exit_code = 0;

int main() {
  // Creates keyvault client and init it
  az_keyvault_keys_client client;
  az_keyvault_keys_client_options options;
  az_span uri = { 0 };

  az_result operation_result = az_keyvault_keys_client_init(&client, uri, &options);
  return exit_code;
}
