// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IDENTITY_ACCESS_TOKEN_CONTEXT_H
#define _az_IDENTITY_ACCESS_TOKEN_CONTEXT_H

#include <az_contract_internal.h>
#include <az_identity_access_token.h>
#include <az_identity_credential.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <_az_cfg_prefix.h>

typedef struct {
  struct {
    az_identity_credential_func credential_func;
    void const * credential;
    az_identity_access_token * token;
    az_span scope;
  } _internal;
} az_identity_access_token_context;

AZ_INLINE AZ_NODISCARD az_result az_identity_access_token_context_init(
    az_identity_access_token_context * const self,
    void const * const credential,
    az_identity_access_token * const token,
    az_span const scope) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(credential);
  AZ_CONTRACT_ARG_NOT_NULL(((az_identity_credential const *)credential)->_internal.func);
  AZ_CONTRACT_ARG_NOT_NULL(token);
  AZ_CONTRACT_ARG_VALID_SPAN(scope);

  *self = (az_identity_access_token_context){
    ._internal = {
      .credential_func = ((az_identity_credential const *)credential)->_internal.func,
      .credential = credential,
      .token = token,
      .scope = scope,
    },
  };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
