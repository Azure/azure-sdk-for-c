// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_iot_provisioning_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

static void test_az_iot_provisioning_client_register_publish_topic_get_succeed(void** state)
{
  (void)state;
}

int test_az_iot_provisioning_client_register(char const* const test_set_name)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_provisioning_client_register_publish_topic_get_succeed),
  };

  return cmocka_run_group_tests_name(test_set_name, tests, NULL, NULL);
}
