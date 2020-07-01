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

#include <azure/core/az_context.h>
#include <azure/core/az_credentials.h>
#include <azure/core/az_http.h>
#include <azure/core/az_json.h>
#include <az_keyvault.h>

// Uncomment below code to enable logging (and the first lines of main function)
#include <azure/core/az_log.h>

// Uncomment below lines when working with libcurl
// #include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>

#define TENANT_ID_ENV "AZURE_TENANT_ID"
#define CLIENT_ID_ENV "AZURE_CLIENT_ID"
#define CLIENT_SECRET_ENV "AZURE_CLIENT_SECRET"
#define URI_ENV "AZURE_KEYVAULT_URL"

// Fuction implementation after main method.
// This function takes the key version from an http keyvault response payload.
az_span get_key_version(az_http_response* response);
// Name for the Key to be used in this sample
az_span const key_name_for_test = AZ_SPAN_LITERAL_FROM_STR("test-new-key");

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif // _MSC_VER

static void test_log_func(az_log_classification classification, az_span message)
{
  (void)classification;
  printf("%.*s\n", az_span_size(message), az_span_ptr(message));
}

int main()
{

  az_log_classification const classifications[] = { AZ_LOG_HTTP_RESPONSE, AZ_LOG_END_OF_LIST };
  az_log_set_classifications(classifications);
  az_log_set_callback(test_log_func)
  // Uncomment below lines when working with libcurl
  /*
    // If running with libcurl, call global init. See project Readme for more info
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
    {
      printf("\nCouldn't init libcurl\n");
      return 1;
    }
    // Set up libcurl cleaning callback as to be called before ending program
    atexit(curl_global_cleanup);
  */

  /************* 1) create secret id credentials for request   ***********/
  az_credential_client_secret credential;

  // init the credential struct
  if (az_credential_client_secret_init(
          &credential,
          az_span_from_str(getenv(TENANT_ID_ENV)),
          az_span_from_str(getenv(CLIENT_ID_ENV)),
          az_span_from_str(getenv(CLIENT_SECRET_ENV)))
      != AZ_OK)
  {
    printf("\nFailed to init credential\n");
    return 1;
  }

  /************ 2) Creates keyvault client    ****************/
  az_keyvault_keys_client client;
  az_keyvault_keys_client_options options = az_keyvault_keys_client_options_default();

  // URL will be copied to client's internal buffer. So we don't need to keep the content of URL
  // buffer immutable  on client's side
  if (az_failed(az_keyvault_keys_client_init(
          &client, az_span_from_str(getenv(URI_ENV)), &credential, &options)))
  {
    printf("\nFailed to init keys client\n");
    return 1;
  }

  /******* 3) Create a buffer for response (will be reused for all requests)   *****/
  uint8_t response_buffer[1024 * 4];
  az_span response_span = AZ_SPAN_FROM_BUFFER(response_buffer);
  az_http_response http_response;
  if (az_http_response_init(&http_response, response_span) != AZ_OK)
  {
    printf("\nFailed to init http response\n");
    return 1;
  }

  /****************** 4) CREATE KEY options for request ******************************/
  az_keyvault_create_key_options key_options = az_keyvault_create_key_options_default();

  // override options values
  az_span operations[2];
  operations[0] = az_keyvault_key_operation_sign();
  operations[1] = AZ_SPAN_NULL;
  key_options.operations = operations;

  // buffer for tags   ->  adding tags
  az_pair tags[3];
  tags[0] = az_pair_from_str("aKey", "aValue");
  tags[1] = az_pair_from_str("bKey", "bValue");
  tags[2] = AZ_PAIR_NULL;
  key_options.tags = tags;

  // 5) This is the actual call to keyvault service
  az_result send_request_result = az_keyvault_keys_key_create(
      &client,
      &az_context_app,
      key_name_for_test,
      az_keyvault_web_key_type_rsa(),
      &key_options,
      &http_response);

  // This validation is only for the first time SDK client is used. API will return not implemented
  // if samples were built with no_http lib.
  if (send_request_result == AZ_ERROR_NOT_IMPLEMENTED)
  {
    printf("Running sample with no_op HTTP implementation.\nRecompile az_core with an HTTP client "
           "implementation like CURL to see sample sending network requests.\n\n"
           "i.e. cmake -DTRANSPORT_CURL=ON ..\n\n");

    return 1;
  }
  else if (send_request_result != AZ_OK) // Any other error would terminate sample
  {
    printf("\nFailed to create key\n");
    return 1;
  }

  printf("Key created: \n%s", response_buffer);

  /****************** 6) GET KEY latest ver ******************************/
  if (az_keyvault_keys_key_get(
          &client, &az_context_app, key_name_for_test, AZ_SPAN_NULL, &http_response)
      != AZ_OK)
  {
    printf("\nFailed to get key\n");
    return 1;
  }

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
          "Version length is greater (%d) than the buffer used to copy it (%d). Use larger buffer.",
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
  if (az_keyvault_keys_key_create(
          &client,
          &az_context_app,
          key_name_for_test,
          az_keyvault_web_key_type_rsa(),
          NULL,
          &http_response)
      != AZ_OK)
  {
    printf("\nFailed to create key version\n");
    return 1;
  }

  printf("\n\n*********************************\nKey new version created: \n%s", response_buffer);

  /****************** 9) GET KEY previous ver ******************************/
  // Here we use the version we recorded previously as parameter
  // Getting a key with NULL parameter for version will return latest version
  if (az_keyvault_keys_key_get(&client, &az_context_app, key_name_for_test, version, &http_response)
      != AZ_OK)
  {
    printf("\nFailed to get previous version of the key\n");
    return 1;
  }

  printf("\n\n*********************************\nGet Key previous Ver: \n%s", response_buffer);

  /****************** 10) DELETE KEY ******************************/
  if (az_keyvault_keys_key_delete(&client, &az_context_app, key_name_for_test, &http_response)
      != AZ_OK)
  {
    printf("\nFailed to delete key\n");
    return 1;
  }

  printf("\n\n*********************************\nDELETED Key: \n %s", response_buffer);

  /****************** 11) GET KEY (should return failed response ) ******************************/
  if (az_keyvault_keys_key_get(
          &client, &az_context_app, key_name_for_test, AZ_SPAN_NULL, &http_response)
      != AZ_OK)
  {
    printf("\nFailed to get key\n");
    return 1;
  }

  printf(
      "\n\n*********************************\nGet Key response again after DELETE: \n%s\n",
      response_buffer);

  return 0;
}

az_span get_key_version(az_http_response* response)
{
  az_span body;

  az_http_response_status_line status_line;
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

  az_span k;
  r = az_json_token_get_string(&value, &k);
  if (az_failed(r))
  {
    return AZ_SPAN_NULL;
  }
  // calculate version
  int32_t const kid_length = az_span_size(k);
  az_span version = AZ_SPAN_NULL;

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
