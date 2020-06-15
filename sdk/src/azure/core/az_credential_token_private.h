// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CREDENTIAL_TOKEN_PRIVATE_H
#define _az_CREDENTIAL_TOKEN_PRIVATE_H

#include <azure/core/az_credentials.h>
#include <azure/core/az_result.h>

#include <stdbool.h>

#include <azure/core/_az_cfg_prefix.h>

AZ_NODISCARD az_result
_az_credential_token_set_token(_az_credential_token* ref_credential, _az_token const* new_token);

AZ_NODISCARD az_result
_az_credential_token_get_token(_az_credential_token* ref_credential, _az_token* out_token);


// Do not invoke on the _az_credential_token directly, i.e. _az_token_expired(&token_credential->_internal.token).
// Instead, call _az_credential_token_get_token() to get the copy first, i.e.:
// _az_token token;
// _az_credential_token_get_token(token_credential, &token);
// _az_token_expired(&token);
AZ_NODISCARD bool _az_token_expired(_az_token const* token);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_CREDENTIAL_TOKEN_PRIVATE_H
