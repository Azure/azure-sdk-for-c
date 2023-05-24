// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "az_test_definitions.h"

int main()
{
  int result = 0;

  // every test function returns the number of tests failed, 0 means success (there shouldn't be
  // negative numbers
  result += test_az_platform();
  result += test_az_mqtt_policy();

  return result;
}
