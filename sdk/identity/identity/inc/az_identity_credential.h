// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IDENTITY_CREDENTIAL_H
#define _az_IDENTITY_CREDENTIAL_H

#include <az_contract.h>
#include <az_http_request_builder.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

typedef AZ_NODISCARD az_result (
    *az_identity_credential_func)(void * const data, az_http_request_builder * const hrb);

typedef struct {
  struct {
    az_identity_credential_func func; // must be the first field in every credential structure
  } _internal;
} az_identity_credential;

AZ_INLINE AZ_NODISCARD az_result _az_identity_credential_init(
    az_identity_credential * const self,
    az_identity_credential_func const credential_func) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(credential_func);

  *self = (az_identity_credential){
    ._internal = { .func = credential_func, },
  };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
