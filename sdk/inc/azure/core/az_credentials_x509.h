// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Credentials used for authentication with many (not all) Azure SDK client libraries.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_CREDENTIALS_X509_H
#define _az_CREDENTIALS_X509_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stddef.h>

#include <azure/core/_az_cfg_prefix.h>

#define AZ_CREDENTIAL_X509_ANONYMOUS NULL

typedef enum
{
  AZ_CREDENTIALS_X509_KEY_MEMORY = 0,
  AZ_CREDENTIALS_X509_KEY_SECURITY_MODULE = 1,
} az_credential_x509_key_type;

typedef struct
{
  az_span cert;
  az_span key;
  az_credential_x509_key_type key_type;
} az_credential_x509;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_CREDENTIALS_X509_H
