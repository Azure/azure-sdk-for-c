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
  az_result creds_retcode = az_client_secret_credential_init(
      &credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  // Create client options
  az_keyvault_keys_client_options client_options;
  az_result client_opts = az_keyvault_keys_client_options_init(&client_options);

  // Init client.
  az_result operation_result = az_keyvault_keys_client_init(
      &client, az_str_to_span(getenv(URI_ENV)), &credential, &client_options);

  // Create a buffer for response
  uint8_t key[1024 * 4];
  az_http_response create_response = { 0 };
  az_result init_http_response_result = az_http_response_init(
      &create_response, az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(key)));

  az_result create_result = az_keyvault_keys_key_create(
      &client, AZ_STR("test-new-key"), AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA, NULL, &create_response);

  printf("Key created:\n %s", key);

  // Creates keyvault client
  az_keyvault_keys_client get_client;

  // create credentials as client_id type
  az_client_secret_credential get_credential = { 0 };
  // init credential_credentials struc
  creds_retcode = az_client_secret_credential_init(
      &get_credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  // Init client with defaults
  operation_result = az_keyvault_keys_client_init(
      &get_client, az_str_to_span(getenv(URI_ENV)), &get_credential, NULL);

  // Create a buffer for response
  uint8_t get_key[1024 * 4];
  az_http_response get_create_response = { 0 };
  init_http_response_result = az_http_response_init(
      &get_create_response, az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(get_key)));

  az_result get_key_result = az_keyvault_keys_key_get(
      &get_client, AZ_STR("test-new-key"), AZ_KEYVAULT_KEY_TYPE_KEY, &get_create_response);

  printf("\n\nGet Key Now:\n %s", get_key);

  return exit_code;
}
