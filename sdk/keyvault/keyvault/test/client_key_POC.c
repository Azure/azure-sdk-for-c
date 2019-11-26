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
  /************ Creates keyvault client    ****************/
  az_keyvault_keys_client client;

  /************* create credentials as client_id type   ***********/
  az_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result creds_retcode = az_client_secret_credential_init(
      &credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  // Init client.
  az_result operation_result
      = az_keyvault_keys_client_init(&client, az_str_to_span(getenv(URI_ENV)), &credential, NULL);

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4];
  az_http_response http_response = { 0 };
  az_result init_http_response_result
      = az_http_response_init(&http_response, (az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer));

  /******************  CREATE KEY ******************************/
  az_result create_result = az_keyvault_keys_key_create(
      &client, AZ_STR("test-new-key"), AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA, NULL, &http_response);

  printf("Key created:\n %s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  init_http_response_result
      = az_http_response_init(&http_response, (az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer));

  /******************  GET KEY ******************************/
  az_result get_key_result = az_keyvault_keys_key_get(
      &client, AZ_STR("test-new-key"), AZ_KEYVAULT_KEY_TYPE_KEY, &http_response);

  printf("\n\nGet Key Now:\n %s", response_buffer);

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  init_http_response_result
      = az_http_response_init(&http_response, (az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer));

  /******************  DELETE KEY ******************************/
  az_result delete_key_result
      = az_keyvault_keys_key_delete(&client, AZ_STR("test-new-key"), &http_response);

  printf("\n\nDELETED Key :\n %s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  init_http_response_result
      = az_http_response_init(&http_response, (az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer));

  /******************  GET KEY (should fail ) ******************************/
  az_result get_key_again_result = az_keyvault_keys_key_get(
      &client, AZ_STR("test-new-key"), AZ_KEYVAULT_KEY_TYPE_KEY, &http_response);

  printf("\n\nGet Key again :\n %s", response_buffer);

  return exit_code;
}
