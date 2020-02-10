// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>
#include <az_keyvault.h>
#include <az_span.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./az_keyvault_create_key_options_test.c"
#include "./az_test.h"

#include <_az_cfg.h>

int exit_code = 0;

AZ_NODISCARD az_result _az_keyvault_keys_key_create_build_json_body(
    az_span json_web_key_type,
    az_keyvault_create_key_options * options,
    az_span * http_body);

int main() {
  {
    {
      az_keyvault_create_key_options options = { 0 };
      uint8_t body_buffer[1024];
      az_span http_body = AZ_SPAN_FROM_BUFFER(body_buffer);

      az_span const expected = AZ_SPAN_FROM_STR("{\"kty\":\"RSA\"}");

      TEST_ASSERT(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_rsa(), &options, &http_body)
          == AZ_OK);

      TEST_ASSERT(az_span_is_equal(http_body, expected));
    }
    {
      az_keyvault_create_key_options options = { 0 };
      options.enabled = az_optional_bool_create(true);

      uint8_t body_buffer[1024];

      az_span http_body = AZ_SPAN_FROM_BUFFER(body_buffer);

      az_span const expected
          = AZ_SPAN_FROM_STR("{\"kty\":\"RSA\",\"attributes\":{\"enabled\":true}}");

      TEST_ASSERT(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_rsa(), &options, &http_body)
          == AZ_OK);

      TEST_ASSERT(az_span_is_equal(http_body, expected));
    }
    {
      az_keyvault_create_key_options options = { 0 };
      options.enabled = az_optional_bool_create(false);

      uint8_t body_buffer[1024];

      az_span http_body = AZ_SPAN_FROM_BUFFER(body_buffer);

      az_span const expected
          = AZ_SPAN_FROM_STR("{\"kty\":\"RSA\",\"attributes\":{\"enabled\":false}}");

      TEST_ASSERT(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_rsa(), &options, &http_body)
          == AZ_OK);

      TEST_ASSERT(az_span_is_equal(http_body, expected));
    }
  }
  az_create_key_options_test();
  return exit_code;
}
