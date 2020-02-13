// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_credentials.h>
#include <az_curl.h>
#include <az_http.h>
#include <az_json.h>
#include <az_keyvault.h>

#include <stdio.h>
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
  az_client_secret_credential credential = { 0 };
  // init credential_credentials struc
  az_result const creds_retcode = az_client_secret_credential_init(
      &credential,
      az_span_from_str(getenv(TENANT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_SECRET_ENV)));

  if (az_failed(creds_retcode)) {
    printf("Failed to init credential");
  }

  az_http_transport_options http_transport_options = { 0 };
  az_result const http_transport_options_init_status
      = az_http_transport_options_init(&http_transport_options);

  if (az_failed(http_transport_options_init_status)) {
    printf("Failed to init http transport options");
  }

  // Init client.
  az_keyvault_keys_client_options options
      = az_keyvault_keys_client_options_default(&http_transport_options);

  // URL will be copied to client's internal buffer. So we don't need to keep the content of URL
  // buffer immutable  on client's side
  az_result const operation_result = az_keyvault_keys_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), &credential, &options);

  if (az_failed(operation_result)) {
    printf("Failed to init keys client");
  }

  /******* Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);
  az_http_response http_response = { 0 };
  az_result const init_http_response_result = az_http_response_init(&http_response, response_span);

  if (az_failed(init_http_response_result)) {
    printf("Failed to init http response");
  }

  /******************  CREATE KEY with options******************************/
  az_keyvault_create_key_options key_options = { 0 };

  // override options values
  key_options.enabled = az_optional_bool_create(false);
  // buffer for operations
  key_options.operations = (az_span[]){ az_keyvault_key_operation_sign(), az_span_null() };

  // buffer for tags   ->  adding tags
  key_options.tags = (az_pair[]){ az_pair_from_str("aKey", "aValue"),
                                  az_pair_from_str("bKey", "bValue"),
                                  az_pair_null() };

  az_result const create_result = az_keyvault_keys_key_create(
      &client,
      AZ_SPAN_FROM_STR("test-new-key"),
      az_keyvault_web_key_type_rsa(),
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

  az_span_set(http_response._internal.http_response, '.');

  /******************  GET KEY latest ver ******************************/
  az_result get_key_result = az_keyvault_keys_key_get(
      &client, AZ_SPAN_FROM_STR("test-new-key"), az_span_null(), &http_response);

  if (az_failed(get_key_result)) {
    printf("Failed to get key");
  }

  printf("\n\n*********************************\nGet Key result: \n%s", response_buffer);

  /****************** get key version from response ****/
  az_span version = get_key_version(&http_response);
  // version is still at http_response. Let's copy it to a new buffer
  uint8_t version_buf[40];
  az_span version_builder = AZ_SPAN_FROM_BUFFER(version_buf);
  az_result const ap_res = az_span_append(version_builder, version, &version_builder);

  if (az_failed(ap_res)) {
    printf("Failed to append key version");
  }

  AZ_RETURN_IF_FAILED(az_span_slice(version_builder, 0, az_span_length(version_builder), &version));

  // Reuse response buffer for delete Key by creating a new span from response_buffer
  az_result const reset2_op = az_http_response_reset(&http_response);
  if (az_failed(reset2_op)) {
    printf("Failed to reset http response (2)");
  }

  az_span_set(response_span, '.');

  /*********************  Create a new key version (use default options) *************/
  az_result const create_version_result = az_keyvault_keys_key_create(
      &client,
      AZ_SPAN_FROM_STR("test-new-key"),
      az_keyvault_web_key_type_rsa(),
      NULL,
      &http_response);

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

  az_span_set(response_span, '.');

  /******************  GET KEY previous ver ******************************/
  az_result const get_key_prev_ver_result = az_keyvault_keys_key_get(
      &client, AZ_SPAN_FROM_STR("test-new-key"), version, &http_response);

  if (az_failed(get_key_prev_ver_result)) {
    printf("Failed to get previous version of the key");
  }

  printf("\n\n*********************************\nGet Key previous Ver: \n%s", response_buffer);

  /******************  DELETE KEY ******************************/
  az_result const delete_key_result
      = az_keyvault_keys_key_delete(&client, AZ_SPAN_FROM_STR("test-new-key"), &http_response);

  if (az_failed(delete_key_result)) {
    printf("Failed to delete key");
  }

  printf("\n\n*********************************\nDELETED Key: \n %s", response_buffer);

  // Reuse response buffer for create Key by creating a new span from response_buffer
  az_result const reset4_op = az_http_response_reset(&http_response);
  if (az_failed(reset4_op)) {
    printf("Failed to reset http response (4)");
  }

  az_span_set(response_span, '.');

  /******************  GET KEY (should return failed response ) ******************************/
  az_result get_key_again_result = az_keyvault_keys_key_get(
      &client, AZ_SPAN_FROM_STR("test-new-key"), az_span_null(), &http_response);

  if (az_failed(get_key_again_result)) {
    printf("Failed to get key (2)");
  }

  printf(
      "\n\n*********************************\nGet Key again after DELETE result: \n%s\n",
      response_buffer);

  return exit_code;
}

az_span get_key_version(az_http_response * response) {
  az_span body = { 0 };

  az_http_response_status_line status_line = { 0 };
  az_result r = az_http_response_get_status_line(response, &status_line);
  if (az_failed(r)) {
    return az_span_null();
  }

  r = az_http_response_get_body(response, &body);
  if (az_failed(r)) {
    return az_span_null();
  }
  // get key from body
  az_json_token value;
  r = az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/key/kid"), &value);
  if (az_failed(r)) {
    return az_span_null();
  }

  az_span k = { 0 };
  r = az_json_token_get_string(value, &k);
  if (az_failed(r)) {
    return az_span_null();
  }
  // calculate version
  int32_t kid_length = az_span_length(k);
  az_span version = { 0 };

  for (int32_t index = kid_length; index > 0; --index) {

    if (az_span_ptr(k)[index] == '/') {
      az_result get_slice_result = az_span_slice(k, index + 1, -1, &version);
      (void)get_slice_result; // if above line fails, version will be a returned empty span (what we
                              // want)
      break;
    }
  }

  return version;
}
