// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_ACCESS_TOKEN_H
#define AZ_ACCESS_TOKEN_H

#include <az_contract.h>
#include <az_credential.h>

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <_az_cfg_prefix.h>

enum {
  _az_ACCESS_TOKEN_TOKEN_BUF_SIZE = 3 * (1024 / 2),
  _az_ACCESS_TOKEN_SCOPE_BUF_SIZE = 100,
};

typedef struct {
  az_credential_func _credential_func;
  az_credential * _credential;
  size_t _token_size;
  size_t _scope_size;
  clock_t _token_expiration;
  uint8_t _token_buf[_az_ACCESS_TOKEN_TOKEN_BUF_SIZE];
  uint8_t _scope_buf[_az_ACCESS_TOKEN_SCOPE_BUF_SIZE];
} az_access_token;

AZ_INLINE AZ_NODISCARD az_result
az_access_token_init(az_access_token * const self, az_credential * const credential) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  *self = (az_access_token){
    ._credential_func = credential->_func,
    ._credential = credential,
    ._token_size = 0,
    ._scope_size = 0,
    ._token_expiration = 0,
    ._token_buf = { 0 },
    ._scope_buf = { 0 },
  };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
