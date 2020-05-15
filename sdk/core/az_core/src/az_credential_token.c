// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_credential_token_private.h"
#include <az_platform.h>

#include <_az_cfg.h>

static bool _az_token_credential_locked = true;
static bool _az_token_credential_unlocked = false;

AZ_NODISCARD az_result _az_credential_token_init(_az_credential_token* out_credential_token)
{
  *out_credential_token = (_az_credential_token){
      ._internal = {
        .lock = &_az_token_credential_unlocked,
        .token = { 0 },
      },
    };

  return AZ_OK;
}

AZ_NODISCARD az_result _az_token_set(_az_credential_token* self, _az_token const* new_token)
{
  while (!az_platform_atomic_compare_exchange(
      &self->_internal.lock, &_az_token_credential_unlocked, &_az_token_credential_locked))
  {
    // Do nothing
  }

  self->_internal.token = *new_token;
  self->_internal.lock = &_az_token_credential_unlocked;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_token_get(_az_credential_token* self, _az_token* out_token)
{
  while (!az_platform_atomic_compare_exchange(
      &self->_internal.lock, &_az_token_credential_unlocked, &_az_token_credential_locked))
  {
    // Do nothing
  }

  *out_token = self->_internal.token;
  self->_internal.lock = &_az_token_credential_unlocked;

  return AZ_OK;
}

AZ_NODISCARD bool _az_token_expired(_az_token const* token)
{
  int64_t const expires_at_msec = token->_internal.expires_at_msec;
  return expires_at_msec <= 0 || az_platform_clock_msec() > expires_at_msec;
}
