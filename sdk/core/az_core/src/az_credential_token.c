// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_credential_token_private.h"
#include <az_platform.h>
#include <az_spinlock_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
_az_credential_token_set_token(_az_credential_token* self, _az_token const* new_token)
{
  _az_spinlock_enter_writer(&self->_internal.lock);
  self->_internal.token = *new_token;
  _az_spinlock_exit_writer(&self->_internal.lock);
  return AZ_OK;
}

AZ_NODISCARD az_result
_az_credential_token_get_token(_az_credential_token* self, _az_token* out_token)
{
  _az_spinlock_enter_reader(&self->_internal.lock);
  *out_token = self->_internal.token;
  _az_spinlock_exit_reader(&self->_internal.lock);
  return AZ_OK;
}

AZ_NODISCARD bool _az_token_expired(_az_token const* token)
{
  int64_t const expires_at_msec = token->_internal.expires_at_msec;
  return expires_at_msec <= 0 || az_platform_clock_msec() > expires_at_msec;
}
