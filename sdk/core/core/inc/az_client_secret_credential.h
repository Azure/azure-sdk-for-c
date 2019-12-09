// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CLIENT_SECRET_CREDENTIAL_H
#define AZ_CLIENT_SECRET_CREDENTIAL_H

#include <_az_token_credential.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  _az_token_credential _token_credential;
  az_span tenant_id;
  az_span client_id;
  az_span client_secret;
} az_client_secret_credential;

AZ_NODISCARD az_result az_client_secret_credential_init(
    az_client_secret_credential * const self,
    az_span const resource,
    az_span const tenant_id,
    az_span const client_id,
    az_span const client_secret);

#include <_az_cfg_suffix.h>

#endif
