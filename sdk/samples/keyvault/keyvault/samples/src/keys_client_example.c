// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file keys_client_example.c
 * @brief
 * Note:
 * This sample requires a KeyVault account set up with soft deleting off to be able to create and
 * delete keys without Purge operation. For Sof-delite keys, purge needs to be done by user after
 * running this sample and before running it again.
 *
 * What this sample does:
 * 1) Creates a secret-id credential getting tenant-id, client-id and secret-id from environment
 * (make sure variables AZURE_TENANT_ID, AZURE_CLIENT_ID and AZURE_CLIENT_SECRET are set).
 *
 * 2) Creates KeyVault SDK client with default options and KeyVault service URL (needs to be
 * set on environment variable AZURE_KEYVAULT_URL)
 *
 * 3) Creates HTTP Response with stack allocated buffer to hold the responses from Service
 *
 * 4) Creates key_options with defaults values and then overrides values for operations and tags
 * with specific values for a new Key to be created with.
 *
 * 5) Calls create Key api using the KeyVault SDK client, http response and key_options.
 * Also adding a name and type for the new key and a global az_context that can be used for
 * cancelling request.
 *
 * 6) Calls get key api using the same http response
 *
 * 7) Parse response payload to get key version. Then copy version from http response to a new stack
 * allocated buffer.
 *
 * 8) Creates a new key version by calling create key with same name
 *
 * 9) Gets previous key version
 *
 * 10) Deletes the key
 *
 * 11) Tries to get key-versions expecting error Not-Found
 *
 */

#include <az_context.h>
#include <az_credentials.h>
#include <az_http.h>
#include <az_json.h>
#include <az_keyvault.h>

#include <stdio.h>
#include <stdlib.h>

#define TENANT_ID_ENV "AZURE_TENANT_ID"
#define CLIENT_ID_ENV "AZURE_CLIENT_ID"
#define CLIENT_SECRET_ENV "AZURE_CLIENT_SECRET"
#define URI_ENV "AZURE_KEYVAULT_URL"

#define RETURN_IF_FAILED(result, message) \
  do \
  { \
    if (az_failed(result)) \
    { \
      printf("\n"); \
      printf(message); \
      printf("\n"); \
      return 1; \
    } \
  } while (0)

az_span get_key_version(az_http_response* response);
az_span const key_name_for_test = AZ_SPAN_LITERAL_FROM_STR("test-new-key");

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
// "Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified"
#pragma warning(disable : 5045)
#endif

int main()
{
  /************* 1) create secret id credentials for request   ***********/
  az_credential_client_secret credential = { { 0 } };

  // init the credential struct
  az_result const credential_init_result = az_credential_client_secret_init(
      &credential,
      az_span_from_str(getenv(TENANT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_ID_ENV)),
      az_span_from_str(getenv(CLIENT_SECRET_ENV)));

  RETURN_IF_FAILED(credential_init_result, "Failed to init credential");

  /************ 2) Creates keyvault client    ****************/
  az_keyvault_keys_client client = { { 0 } };
  az_keyvault_keys_client_options options = az_keyvault_keys_client_options_default();

  // URL will be copied to client's internal buffer. So we don't need to keep the content of URL
  // buffer immutable  on client's side
  az_result const client_init_result = az_keyvault_keys_client_init(
      &client, az_span_from_str(getenv(URI_ENV)), &credential, &options);

  RETURN_IF_FAILED(client_init_result, "Failed to init keys client");

  /******* 3) Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);
  az_http_response http_response = { { 0 } };
  az_result const http_response_init_result = az_http_response_init(&http_response, response_span);

  RETURN_IF_FAILED(http_response_init_result, "Failed to init http response");

  /****************** 4) CREATE KEY with options******************************/
  az_keyvault_create_key_options key_options = az_keyvault_create_key_options_default();

  // override options values
  az_span operations[2] = { { 0 } };
  operations[0] = az_keyvault_key_operation_sign();
  operations[1] = AZ_SPAN_NULL;
  key_options.operations = operations;

  // buffer for tags   ->  adding tags
  az_pair tags[3] = { { 0 } };
  tags[0] = az_pair_from_str("aKey", "aValue");
  tags[1] = az_pair_from_str("bKey", "bValue");
  tags[2] = AZ_PAIR_NULL;
  key_options.tags = tags;

  // 5) This is the actual call to keyvault service
  az_result const key_create_result = az_keyvault_keys_key_create(
      &client,
      &az_context_app,
      key_name_for_test,
      az_keyvault_web_key_type_rsa(),
      &key_options,
      &http_response);

  // validate sample running with no_op http client
  if (key_create_result == AZ_ERROR_NOT_IMPLEMENTED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DTRANSPORT_CURL=ON ..\n\n");

    return 255;
  }

  RETURN_IF_FAILED(key_create_result, "Failed to create key");

  printf("Key created: \n%s", response_buffer);

  /****************** 6) GET KEY latest ver ******************************/
  az_result const key_get_result = az_keyvault_keys_key_get(
      &client, &az_context_app, key_name_for_test, AZ_SPAN_NULL, &http_response);

  RETURN_IF_FAILED(key_get_result, "Failed to get key");

  printf("\n\n*********************************\nGet key: \n%s", response_buffer);

  /****************** 7) get key version from response ****/
  // version is still at http_response. Let's copy it to a new buffer
  uint8_t version_copy_buffer[40] = { 0 };
  az_span version_copy_span = AZ_SPAN_FROM_BUFFER(version_copy_buffer);
  // version span will be pointing to http_response, so it will change as soon as http response is
  // reused. We just need to get it and then copy it to copy_version_builder to keep it
  az_span version = get_key_version(&http_response);
  {
    // make sure that the size of new buffer can hold the version on runtime
    int32_t version_buffer_len = az_span_size(version_copy_span);
    int32_t version_len = az_span_size(version);
    if (version_buffer_len < az_span_size(version))
    {
      printf(
          "Version length is greater (%d) than the buffer used to copy it (%d). "
          "Use larger buffer.",
          version_len,
          version_buffer_len);

      return 1; // Error, terminate proccess with non 0 as error.
    }
    else
    {
      az_span_copy(version_copy_span, version);
      // now let version point to the copy_version_builder and with the right size
      version = az_span_slice(version_copy_span, 0, version_len);
    }
  }

  /*********************  8) Create a new key version (use default options) *************/
  // We previously created a key and saved its version locally
  // We are not re-using same http response to create a new version for same already created key
  // After that, we can get previous version by using the copy of version we got before.
  az_result const version_create_result = az_keyvault_keys_key_create(
      &client,
      &az_context_app,
      key_name_for_test,
      az_keyvault_web_key_type_rsa(),
      NULL,
      &http_response);

  RETURN_IF_FAILED(version_create_result, "Failed to create key version");

  printf("\n\n*********************************\nKey new version created: \n%s", response_buffer);

  /****************** 9) GET KEY previous ver ******************************/
  // Here we use the version we recorded previously as parameter
  // Getting a key with NULL parameter for version will return latest version
  az_result const key_get_prev_ver_result = az_keyvault_keys_key_get(
      &client, &az_context_app, key_name_for_test, version, &http_response);

  RETURN_IF_FAILED(key_get_prev_ver_result, "Failed to get previous version of the key");

  printf("\n\n*********************************\nGet Key previous Ver: \n%s", response_buffer);

  /****************** 10) DELETE KEY ******************************/
  az_result const key_delete_result
      = az_keyvault_keys_key_delete(&client, &az_context_app, key_name_for_test, &http_response);

  RETURN_IF_FAILED(key_delete_result, "Failed to delete key");

  printf("\n\n*********************************\nDELETED Key: \n %s", response_buffer);

  /****************** 11) GET KEY (should return failed response ) ******************************/
  az_result const key_get_again_result = az_keyvault_keys_key_get(
      &client, &az_context_app, key_name_for_test, AZ_SPAN_NULL, &http_response);

  RETURN_IF_FAILED(key_get_again_result, "Failed to get key");

  printf(
      "\n\n*********************************\nGet Key again after DELETE: \n%s\n", response_buffer);

  return 0;
}

az_span get_key_version(az_http_response* response)
{
  az_span body = { 0 };

  az_http_response_status_line status_line = { 0 };
  az_result r = az_http_response_get_status_line(response, &status_line);
  if (az_failed(r))
  {
    return AZ_SPAN_NULL;
  }

  r = az_http_response_get_body(response, &body);
  if (az_failed(r))
  {
    return AZ_SPAN_NULL;
  }
  // get key from body
  az_json_token value;
  r = az_json_parse_by_pointer(body, AZ_SPAN_FROM_STR("/key/kid"), &value);
  if (az_failed(r))
  {
    return AZ_SPAN_NULL;
  }

  az_span k = { 0 };
  r = az_json_token_get_string(&value, &k);
  if (az_failed(r))
  {
    return AZ_SPAN_NULL;
  }
  // calculate version
  int32_t const kid_length = az_span_size(k);
  az_span version = { 0 };

  for (int32_t index = kid_length; index > 0; --index)
  {
    if (az_span_ptr(k)[index] == '/')
    {
      version = az_span_slice_to_end(k, index + 1);
      break;
    }
  }

  return version;
}
