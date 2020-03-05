// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_credential.h
 *
 * @brief definition for az_credential and functionality
 */

#ifndef _az_CREDENTIAL_CLIENT_SECRET_H
#define _az_CREDENTIAL_CLIENT_SECRET_H

#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief a type of az_credential that uses tenant, client and client secret inputs to get
 * authenticated with Azure
 *
 */
typedef struct
{
  struct
  {
    _az_credential credential; // must be the first field in every credential structure
    az_span tenant_id;
    az_span client_id;
    az_span client_secret;
    az_span scopes;
    _az_token token;
  } _internal;
} az_credential_client_secret;

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
AZ_NODISCARD az_result az_credential_client_secret_init(
    az_credential_client_secret* self,
    az_span tenant_id,
    az_span client_id,
    az_span client_secret);

#include <_az_cfg_suffix.h>

#endif // _az_CREDENTIAL_H
