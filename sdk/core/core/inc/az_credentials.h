// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_credentials.h
 *
 * @brief definition for az_credential and functionality
 */

#ifndef _az_CREDENTIALS_H
#define _az_CREDENTIALS_H

#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  _az_TOKEN_BUF_SIZE = 2 * 1024,
};

/**
 * @brief Definition of a auth token. It is used by the auth policy from http pipeline as part of a
 * provided credential. User should not access _internal field.
 *
 */
typedef struct {
  struct {
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
    *_az_credential_apply_fn)(void * credential_options, _az_http_request * ref_request);

/**
 * @brief function callback definition as a contract to be implemented for a credential to set
 * credential scopes when it supports it
 *
 */
typedef AZ_NODISCARD az_result (*_az_credential_set_scopes_fn)(void * credential, az_span scopes);

/**
 * @brief Definition of an az_credential. Its is used internally to authenticate an SDK client with
 * Azure. All types of credentials must contain this structure as it's first type.
 *
 */
typedef struct {
  struct {
    az_http_transport_options http_transport_options;
    _az_credential_apply_fn apply_credential;
    _az_credential_set_scopes_fn set_scopes; // NULL if this credential doesn't support scopes.
  } _internal;
} _az_credential;

/**
 * @brief a type of az_credential that uses tentant, client and client secret inputs to get
 * authenticated with Azure
 *
 */
typedef struct {
  struct {
    _az_credential credential; // must be the first field in every credential structure
    az_span tenant_id;
    az_span client_id;
    az_span client_secret;
    az_span scopes;
    _az_token token;
  } _internal;
} az_client_secret_credential;

/**
 * @brief Initialize a client secret credential with input tenant, client and client secret
 * parameters
 *
 * @param self reference to a client secret credential to initialize
 * @param tenant_id an Azure tenant id
 * @param client_id an Azure client id
 * @param client_secret an Azure client secret
 * @return AZ_OK = Successfull initialization <br>
 * Other value = Initialization failed
 */
AZ_NODISCARD az_result az_client_secret_credential_init(
    az_client_secret_credential * self,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret);

#include <_az_cfg_suffix.h>

#endif
