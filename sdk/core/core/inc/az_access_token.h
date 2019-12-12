// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_ACCESS_TOKEN_H
#define AZ_ACCESS_TOKEN_H

#include <az_contract.h>
#include <az_result.h>

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <_az_cfg_prefix.h>

enum {
  _az_ACCESS_TOKEN_TOKEN_BUF_SIZE = 3 * (1024 / 2),
};

typedef struct {
  size_t _token_size;
  clock_t _token_expiration;
  uint8_t _token_buf[_az_ACCESS_TOKEN_TOKEN_BUF_SIZE];
} az_access_token;

AZ_INLINE AZ_NODISCARD az_result az_access_token_init(az_access_token * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  *self = (az_access_token){
    ._token_size = 0,
    ._token_expiration = 0,
    ._token_buf = { 0 },
  };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
