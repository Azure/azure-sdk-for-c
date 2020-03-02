// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_storage_blobs(void** state)
{
  (void)state;
  // TODO: add storage blobs unit tests
  assert_true(1);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_storage_blobs),
  };

  return cmocka_run_group_tests_name("az_storage_blobs", tests, NULL, NULL);
}
