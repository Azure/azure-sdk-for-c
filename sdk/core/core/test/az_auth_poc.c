// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_auth.h>

#include <az_mut_span.h>
#include <az_str.h>

#include <stdio.h>

#include <_az_cfg.h>

int main() {
  az_result res_code = AZ_OK;

  az_auth_credentials creds = { 0 };
  res_code = az_auth_init_client_credentials(
      &creds,
      AZ_STR("72f988bf-86f1-41af-91ab-2d7cd011db47"),
      AZ_STR("4317a660-6bfb-4585-9ce9-8f222314879c"),
      AZ_STR("O2CT[Y:dkTqblml5V/T]ZEi9x1W1zoBW"));

  if (!az_succeeded(res_code)) {
    printf("az_auth_init_client_credentials failed.\n");
    return 1;
  }

  uint8_t buf[2 * 1024];

  az_mut_span response_buf = { .begin = buf, .size = sizeof(buf) };
  az_span result = { 0 };
  res_code = az_auth_get_token(creds, AZ_STR("https://vault.azure.net"), response_buf, &result);
  if (!az_succeeded(res_code)) {
    printf("az_auth_get_token failed.\n");
    return 1;
  }

  char zero_terminated_token[2 * 1024];
  az_mut_span unused;
  res_code = az_mut_span_to_str(
      (az_mut_span){ .begin = zero_terminated_token, .size = sizeof(zero_terminated_token) },
      result,
      &unused);

  if (!az_succeeded(res_code)) {
    printf("az_mut_span_to_str failed.\n");
    return 1;
  }

  printf("Access_token is: %s\n", zero_terminated_token);

  return 0;
}
