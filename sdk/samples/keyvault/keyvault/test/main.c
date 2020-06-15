// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_json.h>
#include <az_keyvault.h>
#include <azure/core/az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>

#include <cmocka.h>

#include <azure/core/_az_cfg.h>

void test_keyvault(void** state);

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_keyvault),
  };

  return cmocka_run_group_tests_name("az_keyvault", tests, NULL, NULL);
}
