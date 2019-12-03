// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_client_secret_credential.h>
#include <az_http_response_parser.h>
#include <az_json_get.h>
#include <az_json_value.h>
#include <az_keyvault.h>

#include <_az_cfg.h>

#define TENANT_ID_ENV "tenant_id"
#define CLIENT_ID_ENV "client_id"
#define CLIENT_SECRET_ENV "client_secret"
#define URI_ENV "test_uri"

int exit_code = 0;

az_span get_key_version(az_http_response * response) {
  az_http_response_parser parser = { 0 };
  az_span body = { 0 };
  az_result r = az_http_response_parser_init(&parser, az_span_builder_result(&response->builder));

  az_http_response_status_line status_line = { 0 };
  r = az_http_response_parser_read_status_line(&parser, &status_line);

  // Get Body
  r = az_http_response_parser_skip_headers(&parser);
  r = az_http_response_parser_read_body(&parser, &body);

  // get key from body
  az_json_value value;
  r = az_json_get_by_pointer(body, AZ_STR("/key/kid"), &value);

  az_span k = { 0 };
  r = az_json_value_get_string(&value, &k);

  // calculate version
  for (uint8_t index = 0; index < k.size;) {
    ++index;
    if (*(k.begin + k.size - index) == '/') {
      --index;
      k.begin = k.begin + k.size - index;
      k.size = index;
      break;
    }
  }

  return k;
}

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
  az_result init_http_response_result = az_http_response_init(
      &http_response, az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer)));

  /******************  CREATE KEY ******************************/
  az_result create_result = az_keyvault_keys_key_create(
      &client, AZ_STR("test-new-key"), AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA, NULL, &http_response);

  printf("Key created result: \n%s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  az_result reset_op = az_http_response_reset(&http_response);
  az_mut_span_memset(http_response.builder.buffer, '.');

  /******************  GET KEY latest ver ******************************/
  az_result get_key_result
      = az_keyvault_keys_key_get(&client, AZ_STR("test-new-key"), AZ_SPAN_NULL, &http_response);

  printf("\n\n*********************************\nGet Key result: \n%s", response_buffer);

  /****************** get key version from response ****/
  az_span version = get_key_version(&http_response);
  // version is still at http_response. Let's copy it to a new buffer
  uint8_t version_buf[40];
  az_span_builder version_builder
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(version_buf));
  az_result ap_res = az_span_builder_append(&version_builder, version);
  version = az_span_builder_result(&version_builder);

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  reset_op = az_http_response_reset(&http_response);
  az_mut_span_memset(http_response.builder.buffer, '.');

  /*********************  Create a new key version  *************/
  az_result create_version_result = az_keyvault_keys_key_create(
      &client, AZ_STR("test-new-key"), AZ_KEYVAULT_JSON_WEB_KEY_TYPE_RSA, NULL, &http_response);

  printf(
      "\n\n*********************************\nKey new version created result: \n%s",
      response_buffer);

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  reset_op = az_http_response_reset(&http_response);
  az_mut_span_memset(http_response.builder.buffer, '.');

  /******************  GET KEY previous ver ******************************/
  az_result get_key_prev_ver_result
      = az_keyvault_keys_key_get(&client, AZ_STR("test-new-key"), version, &http_response);

  printf("\n\n*********************************\nGet Key previous Ver: \n%s", response_buffer);

  /******************  DELETE KEY ******************************/
  az_result delete_key_result
      = az_keyvault_keys_key_delete(&client, AZ_STR("test-new-key"), &http_response);

  printf("\n\n*********************************\nDELETED Key: \n %s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  reset_op = az_http_response_reset(&http_response);
  az_mut_span_memset(http_response.builder.buffer, '.');

  /******************  GET KEY (should return failed response ) ******************************/
  az_result get_key_again_result
      = az_keyvault_keys_key_get(&client, AZ_STR("test-new-key"), AZ_SPAN_NULL, &http_response);

  printf(
      "\n\n*********************************\nGet Key again after DELETE result: \n%s\n",
      response_buffer);

  return exit_code;
}
