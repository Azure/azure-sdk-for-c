// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_client_secret_credential.h>
#include <az_keyvault.h>

#include <_az_cfg.h>

#define TENANT_ID_ENV "tenant_id"
#define CLIENT_ID_ENV "client_id"
#define CLIENT_SECRET_ENV "client_secret"
#define URI_ENV "test_uri"

int exit_code = 0;

int main() {
  // Creates keyvault client
  az_keyvault_keys_client client;

  // create credentials as client_id type
  az_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result const creds_retcode = az_client_secret_credential_init(
      &credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  // Create client options
  az_keyvault_keys_client_options client_options
      = { .service_version = AZ_STR("7.0"), .retry = { .max_retry = 3, .delay_in_ms = 10 } };

  // Init client
  az_result operation_result = az_keyvault_keys_client_init(
      &client, az_str_to_span(getenv(URI_ENV)), &credential, &client_options);

  // Use client to get a key
  uint8_t key[1024 * 2];
  const az_mut_span key_span = AZ_SPAN_FROM_ARRAY(key);
  az_result get_key_result
      = az_keyvault_keys_key_get(&client, AZ_STR("test-key"), AZ_KEY_VAULT_KEY_TYPE_KEY, &key_span);

  printf("response: %s", key);

  uint8_t same_key[1024 * 2];
  const az_mut_span same_key_span = AZ_SPAN_FROM_ARRAY(same_key);
  // init a new client with default options
  az_keyvault_keys_client client_default_options;
  az_keyvault_keys_client_options default_options;
  operation_result = az_keyvault_keys_client_options_init(&default_options);

  // override one specific value
  default_options.retry.delay_in_ms = 1000;

  // Creating credential again since we can't currently re-use same credential
  // https://github.com/Azure/azure-sdk-for-c/issues/211
  az_client_secret_credential credential2 = { 0 };
  az_result const creds_retcode2 = az_client_secret_credential_init(
      &credential2,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  operation_result = az_keyvault_keys_client_init(
      &client_default_options, az_str_to_span(getenv(URI_ENV)), &credential2, &default_options);

  get_key_result = az_keyvault_keys_key_get(
      &client_default_options, AZ_STR("test-key"), AZ_KEY_VAULT_KEY_TYPE_KEY, &same_key_span);

  printf("\n\n same response: %s", same_key);

  return exit_code;
}
