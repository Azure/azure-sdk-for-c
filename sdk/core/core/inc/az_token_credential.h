// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_TOKEN_CREDENTIAL_H
#define AZ_TOKEN_CREDENTIAL_H

#include <az_credential.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  AZ_TOKEN_CREDENTIAL_TOKEN_BUFFER_SIZE = 2 * 1024,
};

typedef struct {
  az_credential credential; // must be the first field in every credential structure
  uint8_t token[AZ_TOKEN_CREDENTIAL_TOKEN_BUFFER_SIZE]; // TODO: all updates to this must be thread-safe
} az_token_credential;

AZ_NODISCARD az_result
az_token_credential_init(az_token_credential * const self, az_credential_func credential_func);

#include <_az_cfg_suffix.h>

#endif
