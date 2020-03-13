// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_hub_client.h>
#include <az_span.h>

#include <stdio.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

void test_az_iot_hub_client_get_default_options_succeed(void** state)
{
  (void)state;

  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  assert_true(az_span_is_content_equal(options.module_id, AZ_SPAN_NULL));
  assert_true(az_span_is_content_equal(options.user_agent, AZ_SPAN_NULL));
}

int test_iot_hub_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_hub_client_get_default_options_succeed),
  };

  return cmocka_run_group_tests_name("az_iot_client", tests, NULL, NULL);
}
