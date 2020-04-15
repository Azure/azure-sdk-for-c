// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "test_az_iot_core.h"

int main()
{
  int result = 0;

  result += test_az_iot_core("az_iot_core");

  return result;
}
