// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <az_storage_blobs.h>

#include <_az_cfg.h>

void test_storage_blobs_init(void** state);
void test_storage_blobs_init(void** state)
{
  (void)state;
  az_storage_blobs_blob_client client = { 0 };
  az_storage_blobs_blob_client_options opts = az_storage_blobs_blob_client_options_default();

  assert_true(
      az_storage_blobs_blob_client_init(
          &client, AZ_SPAN_FROM_STR("url"), AZ_CREDENTIAL_ANONYMOUS, &opts)
      == AZ_OK);
}
