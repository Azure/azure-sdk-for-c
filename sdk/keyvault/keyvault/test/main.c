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
    az_span const uri = AZ_STR("http://example.net");
    az_span const key_type = AZ_STR("keys");
    az_span const key_name = AZ_STR("name");
    uint8_t out_array[50];
    az_mut_span const out
        = (az_mut_span){ .begin = out_array, .size = uri.size + key_name.size + key_type.size + 2 };

    az_span const expected = AZ_STR("http://example.net/keys/name");

    TEST_ASSERT(az_keyvault_build_url(uri, key_type, key_name, out) == AZ_OK);
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(out), expected));
  }
  return exit_code;
}
