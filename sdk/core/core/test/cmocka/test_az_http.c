// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_http_request(void** state);
void test_http_response(void** state);

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_http_request),
    cmocka_unit_test(test_http_response),
  };

  return cmocka_run_group_tests_name("az_http", tests, NULL, NULL);
}
