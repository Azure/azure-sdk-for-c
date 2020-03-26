// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_iot_tests.h"
#include <az_iot_hub_client.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

int test_iot_provisioning_client()
{
  const struct CMUnitTest tests[] = {
  };
  return cmocka_run_group_tests_name("az_iot_provisioning_client", tests, NULL, NULL);
}
