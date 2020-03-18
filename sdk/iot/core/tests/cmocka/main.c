// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "az_iot_tests.h"

int main()
{
  int result = 0;

  result += test_iot_hub_c2d();
  result += test_iot_hub_client();
  result += test_iot_hub_telemetry();
  result += test_iot_sas_token();

  return result;
}
