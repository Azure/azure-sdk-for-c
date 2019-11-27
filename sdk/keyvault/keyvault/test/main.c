// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "../src/az_keyvault_client.c"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./az_test.h"

#include <_az_cfg.h>

int exit_code = 0;

int main() {
  {
    az_span const kty = AZ_STR("RSA");
    uint8_t body_buffer[1024];
    az_mut_span const span_to_buffer = (az_mut_span)AZ_SPAN_FROM_ARRAY(body_buffer);
    az_span_builder json_builder = az_span_builder_create(span_to_buffer);

    az_span const expected = AZ_STR("{\"kty\":\"RSA\"}");

    TEST_ASSERT(
        build_request_json_body(kty, az_span_builder_append_action(&json_builder)) == AZ_OK);

    az_span const result = az_span_builder_result(&json_builder);

    TEST_ASSERT(az_span_eq(result, expected));
  }
  return exit_code;
}
