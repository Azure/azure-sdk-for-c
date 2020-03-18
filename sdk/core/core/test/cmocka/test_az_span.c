// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <stdarg.h>
#include <stddef.h>

#include <az_span.h>

#include <setjmp.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_az_span_getters(void** state)
{
  (void)state;

  uint8_t example[] = "example";
  az_span span = AZ_SPAN_FROM_INITIALIZED_BUFFER(example);
  assert_int_equal(az_span_capacity(span), 8);
  assert_int_equal(az_span_length(span), 8);
  assert_ptr_equal(az_span_ptr(span), &example);
}
