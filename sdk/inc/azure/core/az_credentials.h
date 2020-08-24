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

#ifndef _az_CREDENTIALS_H
#define _az_CREDENTIALS_H

#include <azure/core/_az_spinlock.h>
#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Equivalent to no credential (`NULL`).
 */
#define AZ_CREDENTIAL_ANONYMOUS NULL

enum
{
  _az_TOKEN_BUFFER_SIZE = 2 * 1024,
};

/**
 * @brief Authentication token.
 *
 * @details It is used by the authentication policy from the HTTP pipeline as part of a provided
 * credential.
 */
typedef struct
{
  struct
  {
    int64_t expires_at_msec;
    int16_t token_length;

    /// Base64-encoded token.
    uint8_t token[_az_TOKEN_BUFFER_SIZE];
  } _internal;
} _az_token;

/**
 * @brief A token credential which pairs authentication token with the thread-safety lock.
 *
 * @remark Users should not access the token directly, without first using the corresponding
 * thread-safe get and set functions which update or get the copy of a token.
 */
typedef struct
{
  struct
  {
    _az_spinlock lock;
    _az_token volatile token;
  } _internal;
} _az_credential_token;

/**
 * @brief Function callback definition as a contract to be implemented for a credential to set
 * authentication scopes when it is supported by the type of the credential.
 */
typedef AZ_NODISCARD az_result (
    *_az_credential_set_scopes_fn)(void* ref_credential, az_span scopes);

/**
 * @brief Credential definition. It is used internally to authenticate an SDK client with Azure. All
 * types of credentials must contain this structure as their first member.
 */
typedef struct
{
  struct
  {
    _az_http_policy_process_fn apply_credential_policy;

    /// If the credential doesn't support scopes, this function pointer is `NULL`.
    _az_credential_set_scopes_fn set_scopes;
  } _internal;
} _az_credential;

/**
 * @brief This structure is used by Azure SDK clients to authenticate with the Azure service using a
 * tenant ID, client ID and client secret.
 */
typedef struct
{
  struct
  {
    /// Must be the first field in every credential structure.
    _az_credential credential;

    _az_credential_token token_credential;
    az_span tenant_id;
    az_span client_id;
    az_span client_secret;
    az_span scopes;
    az_span authority;
  } _internal;
} az_credential_client_secret;

/**
 * @brief Initializes an #az_credential_client_secret instance with the specified tenant ID, client
 * ID, client secret, and Azure AD authority URL.
 *
 * @param[out] out_credential Reference to an #az_credential_client_secret instance to initialize,
 * @param[in] tenant_id An Azure tenant ID.
 * @param[in] client_id An Azure client ID.
 * @param[in] client_secret An Azure client secret.
 * @param[in] authority Authentication authority URL to set. Passing #AZ_SPAN_NULL initializes
 * credential with default authority (Azure AD global authority -
 * "https://login.microsoftonline.com/")
 *
 * @note Example of a non-NULL \p authority string: "https://login.microsoftonline.us/".
 * See national clouds' Azure AD authentication endpoints:
 * https://docs.microsoft.com/en-us/azure/active-directory/develop/authentication-national-cloud.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Initialization failed.
 */
AZ_NODISCARD az_result az_credential_client_secret_init(
    az_credential_client_secret* out_credential,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret,
    az_span authority);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_CREDENTIALS_H
