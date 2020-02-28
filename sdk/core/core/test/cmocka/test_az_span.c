// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdarg.h>
#include <stddef.h>

#include <az_span.h>

#include <setjmp.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_az_span(void** state);
void test_az_span_replace(void** state);

static void test_az_span_getters(void** state)
{
  (void)state;

  uint8_t example[] = "example";
  az_span span = AZ_SPAN_FROM_INITIALIZED_BUFFER(example);
  assert_int_equal(az_span_capacity(span), 8);
  assert_int_equal(az_span_length(span), 8);
  assert_ptr_equal(az_span_ptr(span), &example);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_span_getters),
    cmocka_unit_test(test_az_span),
    cmocka_unit_test(test_az_span_replace),
  };

  return cmocka_run_group_tests_name("az_span", tests, NULL, NULL);
}
