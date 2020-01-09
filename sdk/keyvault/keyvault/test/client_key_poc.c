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
  az_result const creds_retcode = az_identity_client_secret_credential_init(
      &credential,
      az_str_to_span(getenv(TENANT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_ID_ENV)),
      az_str_to_span(getenv(CLIENT_SECRET_ENV)));

  if (az_failed(creds_retcode)) {
    printf("Failed to init credential");
  }

  // Init client.
  az_result const operation_result
      = az_keyvault_keys_client_init(&client, az_str_to_span(getenv(URI_ENV)), &credential, NULL);

  if (az_failed(operation_result)) {
    printf("Failed to init keys client");
  }

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4];
  az_http_response http_response = { 0 };
  az_result const init_http_response_result = az_http_response_init(
      &http_response, az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(response_buffer)));

  if (az_failed(init_http_response_result)) {
    printf("Failed to init http response");
  }

  /******************  CREATE KEY with options******************************/
  az_keyvault_create_key_options key_options = { 0 };

  // override options values
  key_options.enabled = az_optional_bool_create(false);
  // buffer for operations
  az_span operation_buffer[6];
  key_options.operations
      = az_span_span_builder_create((az_mut_span_span)AZ_SPAN_FROM_ARRAY(operation_buffer));

  az_result const append_result = az_keyvault_create_key_options_append_operation(
      &key_options, az_keyvault_key_operation_sign());

  if (az_failed(append_result)) {
    printf("Failed to append operation");
  }

  // buffer for tags   ->  adding tags
  az_pair tags_buffer[5];
  az_pair_span_builder tags_builder
      = az_pair_span_builder_create((az_mut_pair_span)AZ_SPAN_FROM_ARRAY(tags_buffer));

  az_pair tag_a = { .key = AZ_STR("aKey"), .value = AZ_STR("aValue") };
  az_pair tag_b = { .key = AZ_STR("bKey"), .value = AZ_STR("bValue") };
  az_result const adding_tag_a_result = az_pair_span_builder_append(&tags_builder, tag_a);
  az_result const adding_tag_b_result = az_pair_span_builder_append(&tags_builder, tag_b);

  if (az_failed(adding_tag_a_result) || az_failed(adding_tag_b_result)) {
    printf("Failed to append tag");
  }

  key_options.tags = tags_builder;

  az_result const create_result = az_keyvault_keys_key_create(
      &client,
      AZ_STR("test-new-key"),
      az_keyvault_web_key_type_RSA(),
      &key_options,
      &http_response);

  if (az_failed(create_result)) {
    printf("Failed to create key");
  }

  printf("Key created result: \n%s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  az_result const reset1_op = az_http_response_reset(&http_response);
  if (az_failed(reset1_op)) {
    printf("Failed to reset http response (1)");
  }

  az_mut_span_fill(http_response.builder.buffer, '.');

  /******************  GET KEY latest ver ******************************/
  az_result const get_key_result = az_keyvault_keys_key_get(
      &client, AZ_STR("test-new-key"), az_span_empty(), &http_response);

  if (az_failed(get_key_result)) {
    printf("Failed to get key");
  }

  printf("\n\n*********************************\nGet Key result: \n%s", response_buffer);

  /****************** get key version from response ****/
  az_span version = get_key_version(&http_response);
  // version is still at http_response. Let's copy it to a new buffer
  uint8_t version_buf[40];
  az_span_builder version_builder
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(version_buf));
  az_result const ap_res = az_span_builder_append(&version_builder, version);

  if (az_failed(ap_res)) {
    printf("Failed to append key version");
  }

  version = az_span_builder_result(&version_builder);

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  az_result const reset2_op = az_http_response_reset(&http_response);
  if (az_failed(reset2_op)) {
    printf("Failed to reset http response (2)");
  }

  az_mut_span_fill(http_response.builder.buffer, '.');

  /*********************  Create a new key version (use default options) *************/
  az_result const create_version_result = az_keyvault_keys_key_create(
      &client, AZ_STR("test-new-key"), az_keyvault_web_key_type_RSA(), NULL, &http_response);

  if (az_failed(create_version_result)) {
    printf("Failed to create key version");
  }

  printf(
      "\n\n*********************************\nKey new version created result: \n%s",
      response_buffer);

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  az_result const reset3_op = az_http_response_reset(&http_response);
  if (az_failed(reset3_op)) {
    printf("Failed to reset http response (3)");
  }

  az_mut_span_fill(http_response.builder.buffer, '.');

  /******************  GET KEY previous ver ******************************/
  az_result const get_key_prev_ver_result
      = az_keyvault_keys_key_get(&client, AZ_STR("test-new-key"), version, &http_response);

  if (az_failed(get_key_prev_ver_result)) {
    printf("Failed to get previous version of the key");
  }

  printf("\n\n*********************************\nGet Key previous Ver: \n%s", response_buffer);

  /******************  DELETE KEY ******************************/
  az_result const delete_key_result
      = az_keyvault_keys_key_delete(&client, AZ_STR("test-new-key"), &http_response);

  if (az_failed(delete_key_result)) {
    printf("Failed to delete key");
  }

  printf("\n\n*********************************\nDELETED Key: \n %s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  az_result const reset4_op = az_http_response_reset(&http_response);
  if (az_failed(reset4_op)) {
    printf("Failed to reset http response (4)");
  }

  az_mut_span_fill(http_response.builder.buffer, '.');

  /******************  GET KEY (should return failed response ) ******************************/
  az_result const get_key_again_result = az_keyvault_keys_key_get(
      &client, AZ_STR("test-new-key"), az_span_empty(), &http_response);

  if (az_failed(get_key_again_result)) {
    printf("Failed to get key (2)");
  }

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
