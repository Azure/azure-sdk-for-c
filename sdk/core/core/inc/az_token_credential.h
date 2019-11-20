// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_TOKEN_CREDENTIAL_H
#define AZ_TOKEN_CREDENTIAL_H

#include <az_credential.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  AZ_TOKEN_CREDENTIAL_TOKEN_BUFFER_SIZE = 3 * 1024,
};

typedef struct {
  az_credential credential; // must be the first field in every credential structure

  // TODO: all updates to this must be thread-safe
  uint8_t token[AZ_TOKEN_CREDENTIAL_TOKEN_BUFFER_SIZE];
} az_token_credential;

AZ_INLINE AZ_NODISCARD az_result
az_token_credential_init(az_token_credential * const self, az_credential_func credential_func) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  *self = (az_token_credential){ 0 };
  AZ_RETURN_IF_FAILED(az_credential_init(&(self->credential), credential_func));

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
