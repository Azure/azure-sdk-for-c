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
void test_storage_blobs_init_url_host_empty_username_slash(void** state);
void test_storage_blobs_init_url_host_port(void** state);
void test_storage_blobs_init_url_host_port_slash(void** state);

void test_storage_blobs_init_url_too_long(void** state);
void test_storage_blobs_init_credential_error(void** state);

void verify_storage_blobs_upload_empty_host(void** state);

#ifndef AZ_NO_PRECONDITION_CHECKING
void test_storage_blobs_init_null_client(void** state);
void test_storage_blobs_init_bad_url(void** state);

void verify_storage_blobs_upload_null_client(void** state);
void verify_storage_blobs_upload_null_response(void** state);

void verify_storage_blobs_download_null_client(void** state);
void verify_storage_blobs_download_null_response(void** state);
#endif

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
    cmocka_unit_test(test_storage_blobs_init_url_host_username),
    cmocka_unit_test(test_storage_blobs_init_url_host_empty_username_slash),
    cmocka_unit_test(test_storage_blobs_init_url_host_port),
    cmocka_unit_test(test_storage_blobs_init_url_host_port_slash),

    cmocka_unit_test(test_storage_blobs_init_url_too_long),
    cmocka_unit_test(test_storage_blobs_init_credential_error),

    cmocka_unit_test(verify_storage_blobs_upload_empty_host),

#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_storage_blobs_init_null_client),
    cmocka_unit_test(test_storage_blobs_init_bad_url),

    cmocka_unit_test(verify_storage_blobs_upload_null_client),
    cmocka_unit_test(verify_storage_blobs_upload_null_response),

    cmocka_unit_test(verify_storage_blobs_download_null_client),
    cmocka_unit_test(verify_storage_blobs_download_null_response),
#endif
  };

  return cmocka_run_group_tests_name("az_storage_blobs", tests, NULL, NULL);
}
