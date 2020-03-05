// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_credential.h
 *
 * @brief definition for az_credential and functionality
 */

#ifndef _az_CREDENTIAL_H
#define _az_CREDENTIAL_H

#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

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
    uint8_t token[_az_TOKEN_BUF_SIZE]; // Base64-encoded token
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

#define _az_CREDENTIAL_NULL \
  { \
    ._internal = { \
      .apply_credential = NULL, \
      .set_scopes = NULL, \
    }, \
  }

#include <_az_cfg_suffix.h>

#endif // _az_CREDENTIAL_H
