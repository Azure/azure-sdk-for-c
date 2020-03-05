// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_credential_anonymous.h
 *
 * @brief definition for az_credential_anonymous and functionality
 */

#ifndef _az_CRENDENTIAL_ANONYMOUS_H
#define _az_CRENDENTIAL_ANONYMOUS_H

#include <az_credential.h>
#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief a type of az_credential that uses tentant, client and client secret inputs to get
 * authenticated with Azure
 *
 */
typedef struct
{
  struct
  {
    _az_credential credential; // must be the first field in every credential structure
  } _internal;
} az_credential_anonymous;

#define AZ_CREDENTIAL_ANONYMOUS \
  { \
    ._internal = { \
      .credential = _az_CREDENTIAL_NULL, \
    }, \
  }

#include <_az_cfg_suffix.h>

#endif // _az_CRENDENTIAL_ANONYMOUS_H
