// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_TOKEN_CREDENTIAL_H
#define AZ_TOKEN_CREDENTIAL_H

#include <az_credential.h>
#include <az_mut_span.h>
#include <az_span.h>
#include <az_span_builder.h>

#include <time.h>

#include <_az_cfg_prefix.h>

enum {
  _az_TOKEN_CREDENTIAL_TOKEN_BUF_SIZE = 3 * (1024 / 2),
  _az_TOKEN_CREDENTIAL_AUTH_RESOURCE_BUF_SIZE = 100,
};

typedef struct {
  az_credential credential;
  size_t _token_size;
  size_t _resource_size;
  clock_t _token_expiration;
  uint8_t _token_buf[_az_TOKEN_CREDENTIAL_TOKEN_BUF_SIZE];
  uint8_t _resource_buf[_az_TOKEN_CREDENTIAL_AUTH_RESOURCE_BUF_SIZE];
} _az_token_credential;

AZ_INLINE AZ_NODISCARD az_result _az_token_credential_init(
    _az_token_credential * const self,
    az_credential_func const credential_func) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  *self = (_az_token_credential){
    .credential = { 0 },
    ._token_size = 0,
    ._resource_size = 0,
    ._token_expiration = 0,
    ._token_buf = { 0 },
    ._resource_buf = { 0 },
  };

  return _az_credential_init(&(self->credential), credential_func);
}

#include <_az_cfg_suffix.h>

#endif
