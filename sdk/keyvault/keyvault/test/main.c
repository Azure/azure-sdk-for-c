// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>
#include <az_keyvault.h>
#include <az_mut_span.h>
#include <az_span_builder.h>
#include <az_span_builder_internal.h>

#include <az_keyvault_client_private.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./az_keyvault_create_key_options_test.c"
#include "./az_test.h"

#include <_az_cfg.h>

int exit_code = 0;

int main() {
  {
    {
      az_keyvault_create_key_options options = { 0 };
      uint8_t body_buffer[1024];
      az_mut_span const span_to_buffer = (az_mut_span)AZ_SPAN_FROM_ARRAY(body_buffer);
      az_span_builder json_builder = az_span_builder_create(span_to_buffer);

      az_span const expected = AZ_STR("{\"kty\":\"RSA\"}");

      TEST_ASSERT(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_RSA(),
              &options,
              az_span_builder_append_action(&json_builder))
          == AZ_OK);

      az_span result = az_span_builder_result(&json_builder);

      TEST_ASSERT(az_span_is_equal(result, expected));
    }
    {
      az_keyvault_create_key_options options = { 0 };
      options.enabled = az_optional_bool_create(true);

      uint8_t body_buffer[1024];

      az_mut_span const span_to_buffer = (az_mut_span)AZ_SPAN_FROM_ARRAY(body_buffer);
      az_span_builder json_builder = az_span_builder_create(span_to_buffer);

      az_span const expected = AZ_STR("{\"kty\":\"RSA\",\"attributes\":{\"enabled\":true}}");

      TEST_ASSERT(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_RSA(),
              &options,
              az_span_builder_append_action(&json_builder))
          == AZ_OK);

      az_span result = az_span_builder_result(&json_builder);

      TEST_ASSERT(az_span_is_equal(result, expected));
    }
    {
      az_keyvault_create_key_options options = { 0 };
      options.enabled = az_optional_bool_create(false);

      uint8_t body_buffer[1024];

      az_mut_span const span_to_buffer = (az_mut_span)AZ_SPAN_FROM_ARRAY(body_buffer);
      az_span_builder json_builder = az_span_builder_create(span_to_buffer);

      az_span const expected = AZ_STR("{\"kty\":\"RSA\",\"attributes\":{\"enabled\":false}}");

      TEST_ASSERT(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_RSA(),
              &options,
              az_span_builder_append_action(&json_builder))
          == AZ_OK);

      az_span result = az_span_builder_result(&json_builder);

      TEST_ASSERT(az_span_is_equal(result, expected));
    }
  }
  az_create_key_options_test();
  return exit_code;
}
