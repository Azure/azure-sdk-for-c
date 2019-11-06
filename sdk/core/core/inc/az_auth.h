// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_AUTH_H
#define AZ_AUTH_H

#include <az_mut_span.h>
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_AUTH_KIND_CLIENT_CREDENTIALS = 1,
} az_auth_kind;

typedef struct {
  az_span tenant_id;
  az_auth_kind kind;
  union {
    struct {
      az_span client_id;
      az_span client_secret;
    } client_credentials;
  } data;
} az_auth_credentials;

AZ_NODISCARD az_result az_auth_init_client_credentials(
    az_auth_credentials * const out_result,
    az_span const tenant_id,
    az_span const client_id,
    az_span const client_secret);

AZ_NODISCARD az_result az_auth_get_token(
    az_auth_credentials const credentials,
    az_span const resource_url,
    az_mut_span const response_buf,
    az_span * const out_result);

#include <_az_cfg_suffix.h>

#endif
