// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "az_test_definitions.h"

#include <_az_cfg.h>

int main()
{
  int result = 0;

  result += test_az_json();
  result += test_az_span();
  result += test_az_context();
  result += test_az_http();
  result += test_az_logging();
  result += test_az_encode();
  result += test_az_pipeline();
  result += test_az_add();
  result += test_az_policy();

  return result;
}
