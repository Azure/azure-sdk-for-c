// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <azure/core/_az_cfg.h>

void test_storage_blobs_init(void** state);
void test_storage_blobs_upload(void** state);
void test_storage_blobs_download(void** state);

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_storage_blobs_init),
    cmocka_unit_test(test_storage_blobs_upload),
    cmocka_unit_test(test_storage_blobs_download),
  };

  return cmocka_run_group_tests_name("az_storage_blobs", tests, NULL, NULL);
}
