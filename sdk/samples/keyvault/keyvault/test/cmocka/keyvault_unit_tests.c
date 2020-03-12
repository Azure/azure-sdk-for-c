// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>
#include <az_keyvault.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result _az_keyvault_keys_key_create_build_json_body(
    az_span json_web_key_type,
    az_keyvault_create_key_options* options,
    az_span* http_body);

void test_keyvault(void** state)
{
  (void)state;
  {
    {
      az_keyvault_create_key_options options = { 0 };
      uint8_t body_buffer[1024];
      az_span http_body = AZ_SPAN_FROM_BUFFER(body_buffer);

      az_span const expected = AZ_SPAN_FROM_STR("{\"kty\":\"RSA\"}");

      assert_true(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_rsa(), &options, &http_body)
          == AZ_OK);

      assert_true(az_span_is_content_equal(http_body, expected));
    }
    {
      az_keyvault_create_key_options options = { 0 };
      options.enabled = az_optional_bool_create(true);

      uint8_t body_buffer[1024];

      az_span http_body = AZ_SPAN_FROM_BUFFER(body_buffer);

      az_span const expected
          = AZ_SPAN_FROM_STR("{\"kty\":\"RSA\",\"attributes\":{\"enabled\":true}}");

      assert_true(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_rsa(), &options, &http_body)
          == AZ_OK);

      assert_true(az_span_is_content_equal(http_body, expected));
    }
    {
      az_keyvault_create_key_options options = { 0 };
      options.enabled = az_optional_bool_create(false);

      uint8_t body_buffer[1024];

      az_span http_body = AZ_SPAN_FROM_BUFFER(body_buffer);

      az_span const expected
          = AZ_SPAN_FROM_STR("{\"kty\":\"RSA\",\"attributes\":{\"enabled\":false}}");

      assert_true(
          _az_keyvault_keys_key_create_build_json_body(
              az_keyvault_web_key_type_rsa(), &options, &http_body)
          == AZ_OK);

      assert_true(az_span_is_content_equal(http_body, expected));
    }
  }
}
