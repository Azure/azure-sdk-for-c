// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_AUTH_H
#define AZ_AUTH_H

#include <az_http_request_builder.h>
#include <az_result.h>
#include <az_span.h>
#include <az_mut_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span tenant_id;
  az_span client_id;
  az_span client_secret;
} az_auth_client_credentials;

typedef AZ_NODISCARD az_result (*az_auth_func)(
  void * const data,
  az_mut_span const buffer,
  az_http_request_builder * const hrb);

typedef struct {
  void * data;
  az_auth_func func;
} az_auth_callback;

AZ_NODISCARD az_result az_auth_init_client_credentials(
    az_auth_callback * const out_callback,
    az_auth_client_credentials * const out_data,
    az_span const tenant_id,
    az_span const client_id,
    az_span const client_secret);

#include <_az_cfg_suffix.h>

#endif
