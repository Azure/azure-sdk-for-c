// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_token_credential.h>

#include <az_contract.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_token_credential_init(az_token_credential * const self, az_credential_func credential_func) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  *self = (az_token_credential){ 0 };
  AZ_RETURN_IF_FAILED(az_credential_init(&(self->credential), credential_func));

  return AZ_OK;
}
