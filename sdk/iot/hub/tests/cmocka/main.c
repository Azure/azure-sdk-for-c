// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "test_az_iot_hub_client.h"

int main()
{
  int result = 0;

  result += test_iot_hub_c2d("az_iot_hub_c2d");
  result += test_iot_hub_client("az_iot_hub_client");
  result += test_iot_sas_token("az_iot_hub_client_sas");
  result += test_iot_hub_telemetry("az_iot_hub_client_telemetry");
  result += test_az_iot_hub_client_twin("az_iot_hub_client_twin");
  result += test_iot_hub_methods("az_iot_hub_methods");

  return result;
}
