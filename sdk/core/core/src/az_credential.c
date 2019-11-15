// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_credential.h>
#include <az_result.h>

#include <az_contract.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_credential_init(az_credential * const self, az_credential_func credential_func) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(credential_func);

  *self = (az_credential){ .func = credential_func };

  return AZ_OK;
}
