// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <azure/core/_az_cfg.h>

void test_storage_blobs_init(void** state);
void test_storage_blobs_init_nonnull_options(void** state);
void test_storage_blobs_upload(void** state);
void test_storage_blobs_download(void** state);

void test_storage_blobs_init_url_no_colon(void** state);
void test_storage_blobs_init_url_no_slash1(void** state);
void test_storage_blobs_init_url_no_slash2(void** state);
void test_storage_blobs_init_url_empty_host_slash(void** state);
void test_storage_blobs_init_url_empty_host_username(void** state);
void test_storage_blobs_init_url_host_username(void** state);
void test_storage_blobs_init_url_host_empty_username_with_slash(void** state);
void test_storage_blobs_init_url_host_port(void** state);

void test_storage_blobs_init_url_too_long(void** state);
void test_storage_blobs_init_credential_error(void** state);

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_storage_blobs_init),
    cmocka_unit_test(test_storage_blobs_init_nonnull_options),
    cmocka_unit_test(test_storage_blobs_upload),
    cmocka_unit_test(test_storage_blobs_download),

    cmocka_unit_test(test_storage_blobs_init_url_no_colon),
    cmocka_unit_test(test_storage_blobs_init_url_no_slash1),
    cmocka_unit_test(test_storage_blobs_init_url_no_slash2),
    cmocka_unit_test(test_storage_blobs_init_url_empty_host_slash),
    cmocka_unit_test(test_storage_blobs_init_url_empty_host_username),
    cmocka_unit_test(test_storage_blobs_init_url_host_empty_username_with_slash),

    cmocka_unit_test(test_storage_blobs_init_url_too_long),
    cmocka_unit_test(test_storage_blobs_init_credential_error),
  };

  return cmocka_run_group_tests_name("az_storage_blobs", tests, NULL, NULL);
}
