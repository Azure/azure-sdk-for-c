// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_IDENTITY_ACCESS_TOKEN_CONTEXT_H
#define AZ_IDENTITY_ACCESS_TOKEN_CONTEXT_H

#include <az_contract.h>
#include <az_identity_access_token.h>
#include <az_identity_credential.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_identity_credential_func _credential_func;
  void const * _credential;
  az_identity_access_token * _token;
  az_span _scope;
} az_identity_access_token_context;

AZ_INLINE AZ_NODISCARD az_result az_identity_access_token_context_init(
    az_identity_access_token_context * const self,
    void const * const credential,
    az_identity_access_token * const token,
    az_span const scope) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(credential);
  AZ_CONTRACT_ARG_NOT_NULL(((az_identity_credential const *)credential)->_func);
  AZ_CONTRACT_ARG_NOT_NULL(token);
  AZ_CONTRACT_ARG_VALID_SPAN(scope);

  *self = (az_identity_access_token_context){
    ._credential_func = ((az_identity_credential const *)credential)->_func,
    ._credential = credential,
    ._token = token,
    ._scope = scope,
  };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
