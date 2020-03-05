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

void test_keyvault(void** state);

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_keyvault),
  };

  return cmocka_run_group_tests_name("az_keyvault", tests, NULL, NULL);
}
