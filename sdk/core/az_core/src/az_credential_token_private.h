// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CREDENTIAL_TOKEN_PRIVATE_H
#define _az_CREDENTIAL_TOKEN_PRIVATE_H

#include <az_credentials.h>
#include <az_result.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result _az_credential_token_init(_az_credential_token* out_credential_token);

AZ_NODISCARD az_result _az_credential_token_set_credential(_az_credential_token* self, _az_token const* new_token);
AZ_NODISCARD az_result _az_credential_token_get_credential(_az_credential_token* self, _az_token* out_token);

AZ_NODISCARD bool _az_token_expired(_az_token const* token);

#include <_az_cfg_suffix.h>

#endif // _az_CREDENTIAL_TOKEN_PRIVATE_H
