// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_credentials.h
 *
 * @brief Credentials used for authentication with many (not all) Azure SDK client libraries.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_CREDENTIALS_H
#define _az_CREDENTIALS_H

#include <az_http.h>
#include <az_http_transport.h>
#include <az_result.h>
#include <az_span.h>

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief AZ_CREDENTIAL_ANONYMOUS is equivallent to no credential (NULL).
 *
 */
#define AZ_CREDENTIAL_ANONYMOUS NULL

enum
{
  _az_TOKEN_BUF_SIZE = 2 * 1024,
};

/**
 * @brief Definition of a auth token. It is used by the auth policy from http pipeline as part of a
 * provided credential. User should not access _internal field.
 *
 */
typedef struct
{
  struct
  {
    uint8_t token[_az_TOKEN_BUF_SIZE]; /*!< Base64-encoded token */
    int16_t token_length;
    int64_t expires_at_msec;
  } _internal;
} _az_token;

/**
 * @brief function callback definition as a contract to be implemented for a credential
 *
 */
typedef AZ_NODISCARD az_result (
    *_az_credential_apply_fn)(void* credential_options, _az_http_request* ref_request);

/**
 * @brief function callback definition as a contract to be implemented for a credential to set
 * credential scopes when it supports it
 *
 */
typedef AZ_NODISCARD az_result (*_az_credential_set_scopes_fn)(void* credential, az_span scopes);

/**
 * @brief Definition of an az_credential. Its is used internally to authenticate an SDK client with
 * Azure. All types of credentials must contain this structure as it's first type.
 *
 */
typedef struct
{
  struct
  {
    _az_credential_apply_fn apply_credential;
    _az_credential_set_scopes_fn set_scopes; // NULL if this credential doesn't support scopes.
  } _internal;
} _az_credential;

/**
 * @brief This structure is used by Azure SDK clients to authenticate with the
 * Azure service using a tenant ID, client ID and client secret.
 */
typedef struct
{
  struct
  {
    _az_credential credential; /// must be the first field in every credential structure
    az_span tenant_id;
    az_span client_id;
    az_span client_secret;
    az_span scopes;
    _az_token token;
  } _internal;
} az_credential_client_secret;

/**
 * @brief az_credential_client_secret_init initializes an az_credential_client_secret instance
 * with the specified tenant ID, client ID and client secret.
 *
 * @param self reference to a az_credential_client_secret instance to initialize
 * @param tenant_id an Azure tenant ID
 * @param client_id an Azure client ID
 * @param client_secret an Azure client secret
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 *         - Other error code if initialization failed
 */
AZ_NODISCARD az_result az_credential_client_secret_init(
    az_credential_client_secret* self,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret);

#include <_az_cfg_suffix.h>

#endif // _az_CREDENTIALS_H
