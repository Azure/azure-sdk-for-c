// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Azure x509 credential definitions.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_PLATFORM_X509_CREDENTIAL_H
#define _az_PLATFORM_X509_CREDENTIAL_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_credentials_common.h>

#include <stddef.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Defines the key storage types.
 */
typedef enum
{
  AZ_CREDENTIALS_X509_KEY_MEMORY = 0,
  AZ_CREDENTIALS_X509_KEY_SECURITY_MODULE = 1,
} az_platform_credential_x509_key_type;

/**
 * @brief x509 credential definition. 
 */
typedef struct
{
  az_span cert;
  az_span key;
  az_platform_credential_x509_key_type key_type;
} az_platform_x509_credential;

/**
 * @brief x509 credential constructor. 
 */
AZ_NODISCARD AZ_INLINE az_credential az_credential_x509_init(az_platform_x509_credential* cred)
{ 
  return (az_credential){ 
    .type = AZ_CREDENTIAL_X509,
    ._internal = {
      .apply_credential_policy = NULL,
      .set_scopes = NULL,
      .credential = (az_platform_credential) cred, 
    }
  };
}

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_X509_CREDENTIAL_H
