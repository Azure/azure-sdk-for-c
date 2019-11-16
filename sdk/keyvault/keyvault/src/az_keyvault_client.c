// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <_az_cfg.h>

static uint16_t const MAX_URL_SIZE = 80;

az_span const AZ_KEY_VAULT_NONE_TYPE = AZ_CONST_STR("");
az_span const AZ_KEY_VAULT_KEY_TYPE = AZ_CONST_STR("keys");
az_span const AZ_KEY_VAULT_SECRET_TYPE = AZ_CONST_STR("secrets");
az_span const AZ_KEY_VAULT_CERTIFICATE_TYPE = AZ_CONST_STR("certificates");
az_span const AZ_KEY_VAULT_URL_PATH_SEPARATOR = AZ_CONST_STR("/");

// TODO #define AZ_KEYVAULT_KEYS_KEYTYPE_XXXX
// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_createKey(
    az_keyvault_keys_client * client,
    az_span keyname,
    az_span keytype,
    az_keyvault_keys_keys_options * options,
    const az_mut_span * const out) {
  (void)client;
  (void)keyname;
  (void)keytype;
  (void)options;
  (void)out;
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_build_url(
    az_span const uri,
    az_span const key_type,
    az_span const key_name,
    az_mut_span const out) {
  az_span_builder s_builder = az_span_builder_create(out);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, uri));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, AZ_KEY_VAULT_URL_PATH_SEPARATOR));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, key_type));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, AZ_KEY_VAULT_URL_PATH_SEPARATOR));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&s_builder, key_name));
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_keys_getKey(
    az_keyvault_keys_client const * const client,
    az_span const keyname,
    az_key_vault_key_type const keytype,
    const az_mut_span * const out) {
  // create request buffer TODO: define size for a getKey Request
  uint8_t request_buffer[1024 * 4];
  az_mut_span request_buffer_span = AZ_SPAN_FROM_ARRAY(request_buffer);

  /* ******** build url for request  ******
   * add keytype, keyname and version to url request
   */
  az_span const az_key_type_span = az_keyvault_get_key_type_span(keytype);

  // make sure we can handle url within the MAX_URL_SIZE
  size_t const url_size_with_path = client->uri.size + (AZ_KEY_VAULT_URL_PATH_SEPARATOR.size * 2)
      + az_key_type_span.size + keyname.size;
  AZ_CONTRACT(url_size_with_path < MAX_URL_SIZE, AZ_ERROR_BUFFER_OVERFLOW);

  // stack a new buffer to hold url with key name and type
  uint8_t url_buffer[MAX_URL_SIZE];
  az_mut_span const url_buffer_span
      = (az_mut_span){ .begin = url_buffer, .size = url_size_with_path };
  AZ_RETURN_IF_FAILED(
      az_keyvault_build_url(client->uri, az_key_type_span, keyname, url_buffer_span));

  // create request
  // TODO: define max URL size
  az_http_request_builder hrb;
  AZ_RETURN_IF_FAILED(az_http_request_builder_init(
      &hrb,
      request_buffer_span,
      MAX_URL_SIZE,
      AZ_HTTP_METHOD_VERB_GET,
      az_mut_span_to_span(url_buffer_span)));

  // add version to request
  AZ_RETURN_IF_FAILED(
      az_http_request_builder_set_query_parameter(&hrb, AZ_STR("api-version"), client->version));

  // policies
  az_http_policies policies = { 0 };
  az_result policies_retcode = az_http_policies_init(&policies);
  if (!az_succeeded(policies_retcode)) {
    printf("Error initializing policies\n");
    return policies_retcode;
  }

  policies.authentication.data = client->pipeline_policies.pipeline[2].data;

  // start pipeline
  return az_http_pipeline_process(&hrb, out, &policies);
}
