// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <_az_cfg.h>

int exit_code = 0;

int main() {
  // Creates keyvault client
  az_keyvault_keys_client client;

  // create credentials as client_id type
  az_auth_credentials auth = { 0 };
  // Get secrets from ENV
  char * const TENANT_ID = getenv("tenant_id");
  char * const CLIENT_ID = getenv("client_id");
  char * const CLIENT_SECRET = getenv("client_secret");
  // init auth_credentials struc
  az_result const creds_retcode = az_auth_init_client_credentials(
      &auth,
      (az_span){ .begin = TENANT_ID, .size = strlen(TENANT_ID) },
      (az_span){ .begin = CLIENT_ID, .size = strlen(CLIENT_ID) },
      (az_span){ .begin = CLIENT_SECRET, .size = strlen(CLIENT_SECRET) });

  // Create client options
  az_keyvault_keys_client_options options = { .version = AZ_STR("7.0") };

  // Init client
  // TODO: introduce init and init_with_options so options can be optional for user
  az_result operation_result = az_keyvault_keys_client_init(
      &client, AZ_STR("https://testingforc99.vault.azure.net"), &auth, &options);

  // Use client to get a key
  uint8_t key[200];
  const az_mut_span key_span = AZ_SPAN_FROM_ARRAY(key);
  az_result get_key_result
      = az_keyvault_keys_getKey(&client, AZ_STR("test-key"), AZ_KEY_VAULT_KEY, &key_span);

  printf("response: %s", key);

  return exit_code;
}
