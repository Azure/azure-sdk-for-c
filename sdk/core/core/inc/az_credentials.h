// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CREDENTIALS_H
#define _az_CREDENTIALS_H

#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  _az_MAX_TOKEN_LENGTH = 2 * 1024,
};

typedef struct {
  struct {
    uint8_t token[_az_MAX_TOKEN_LENGTH]; // Base64-encoded token
    int16_t token_length;
    int64_t expires_at_msec;
  } _internal;
} _az_token;

typedef struct {
  struct {
    _az_credential_vtbl vtbl; // must be the first field in every credential structure
    az_span tenant_id;
    az_span client_id;
    az_span client_secret;
    az_span scopes;
    _az_token token;
  } _internal;
} az_client_secret_credential;

AZ_NODISCARD az_result az_client_secret_credential_init(
    az_client_secret_credential * self,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret);

typedef AZ_NODISCARD az_result (*_az_credential_set_scopes_fn)(void * credential, az_span scopes);

typedef struct {
  _az_apply_credential apply_credential;
  // NULL if this credential doesn't support scopes.
  _az_credential_set_scopes
      set_scopes;
} _az_credential_vtbl;

#include <_az_cfg_suffix.h>

#endif
