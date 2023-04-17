// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Azure IoT x509 certificate definitions.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_X509_CERTIFICATE_H
#define _az_IOT_X509_CERTIFICATE_H

#include <azure/core/az_span.h>

#include <stddef.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Defines the key storage types.
 */
typedef enum
{
  AZ_IOT_X509_CERTIFICATE_KEY_STORAGE_MEMORY = 0,
  AZ_IOT_X509_CERTIFICATE_KEY_STORAGE_SECURITY_MODULE = 1,
} az_iot_x509_certificate_key_source;

/**
 * @brief x509 certificate definition.
 */
typedef struct
{
  struct
  {
    /// Contains x509 certificate
    az_span certificate;

    /// Contains x509 private key
    az_span key;

    /// Specifies the storage type for the key.
    az_iot_x509_certificate_key_source key_source;
  } _internal;
} az_iot_x509_certificate;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_X509_CERTIFICATE_H
