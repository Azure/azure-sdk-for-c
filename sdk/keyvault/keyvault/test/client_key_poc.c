// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>
#include <az_identity_client_secret_credential.h>
#include <az_json_get.h>
#include <az_json_token.h>
#include <az_keyvault.h>

#include <stdlib.h>

#include <_az_cfg.h>

#define TENANT_ID_ENV "tenant_id"
#define CLIENT_ID_ENV "client_id"
#define CLIENT_SECRET_ENV "client_secret"
#define URI_ENV "test_uri"

int exit_code = 0;

az_span get_key_version(az_http_response * response);

int main() {
  /************ Creates keyvault client    ****************/
  az_keyvault_keys_client client;

  /************* create credentials as client_id type   ***********/
  az_identity_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result creds_retcode = az_identity_client_secret_credential_init(
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

  /******************  CREATE KEY with options******************************/
  az_keyvault_create_key_options key_options = { 0 };

  // override options values
  key_options.enabled = az_optional_bool_create(false);
  // buffer for operations
  az_span operation_buffer[6];
  key_options.operations
      = az_span_span_builder_create((az_mut_span_span)AZ_SPAN_FROM_ARRAY(operation_buffer));
  az_result append_result = az_keyvault_create_key_options_append_operation(
      &key_options, az_keyvault_key_operation_sign());

  // buffer for tags   ->  adding tags
  az_pair tags_buffer[5];
  az_pair_span_builder tags_builder
      = az_pair_span_builder_create((az_mut_pair_span)AZ_SPAN_FROM_ARRAY(tags_buffer));

  az_pair tag_a = { .key = AZ_STR("aKey"), .value = AZ_STR("aValue") };
  az_pair tag_b = { .key = AZ_STR("bKey"), .value = AZ_STR("bValue") };
  az_result adding_tag_result = az_pair_span_builder_append(&tags_builder, tag_a);
  adding_tag_result = az_pair_span_builder_append(&tags_builder, tag_b);

  key_options.tags = tags_builder;

  az_result create_result = az_keyvault_keys_key_create(
      &client,
      AZ_STR("test-new-key"),
      az_keyvault_web_key_type_RSA(),
      &key_options,
      &http_response);

  printf("Key created result: \n%s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  az_result reset_op = az_http_response_reset(&http_response);
  az_mut_span_fill(http_response.builder.buffer, '.');

  /******************  GET KEY latest ver ******************************/
  az_result get_key_result = az_keyvault_keys_key_get(
      &client, AZ_STR("test-new-key"), az_span_create_empty(), &http_response);

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
  az_mut_span_fill(http_response.builder.buffer, '.');

  /*********************  Create a new key version (use default options) *************/
  az_result create_version_result = az_keyvault_keys_key_create(
      &client, AZ_STR("test-new-key"), az_keyvault_web_key_type_RSA(), NULL, &http_response);

  printf(
      "\n\n*********************************\nKey new version created result: \n%s",
      response_buffer);

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  reset_op = az_http_response_reset(&http_response);
  az_mut_span_fill(http_response.builder.buffer, '.');

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
  az_mut_span_fill(http_response.builder.buffer, '.');

  /******************  GET KEY (should return failed response ) ******************************/
  az_result get_key_again_result = az_keyvault_keys_key_get(
      &client, AZ_STR("test-new-key"), az_span_create_empty(), &http_response);

  printf(
      "\n\n*********************************\nGet Key again after DELETE result: \n%s\n",
      response_buffer);

  return exit_code;
}

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
  az_json_token value;
  r = az_json_get_by_pointer(body, AZ_STR("/key/kid"), &value);

  az_span k = { 0 };
  r = az_json_token_get_string(value, &k);

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
