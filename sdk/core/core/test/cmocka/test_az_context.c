// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_context.h>
#include <az_result.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_az_context(void** state)
{
  (void)state;

  void* const key = "k";
  void* value = "v";
  az_context ctx1 = az_context_with_expiration(&az_context_app, 100);
  az_context ctx2 = az_context_with_value(&ctx1, key, value);
  az_context ctx3 = az_context_with_expiration(&ctx2, 250);

  int64_t expiration = az_context_get_expiration(&ctx3);
  void* value2 = NULL;
  az_result r = az_context_get_value(&ctx3, key, &value2);

  assert_true(r == AZ_OK);
  assert_true(value2 != NULL);

  r = az_context_get_value(&ctx3, "", &value2);

  assert_true(r == AZ_ERROR_ITEM_NOT_FOUND);
  assert_true(value2 == NULL);

  az_context_cancel(&ctx1);
  expiration = az_context_get_expiration(&ctx3); // Should be 0

  assert_true(expiration == 0);
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_az_context) };

  return cmocka_run_group_tests_name("az_context", tests, NULL, NULL);
}
