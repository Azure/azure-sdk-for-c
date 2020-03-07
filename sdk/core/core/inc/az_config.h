// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_config.h
 *
 * @brief Configurable constants for SDK behavior.
 */

#ifndef _az_CONFIG_H
#define _az_CONFIG_H

#include <_az_cfg_prefix.h>

enum
{
  AZ_HTTP_REQUEST_URL_BUF_SIZE = 2 * 1024, ///< Default maximum length a URL can have.
  AZ_HTTP_REQUEST_BODY_BUF_SIZE = 1024, ///< Default maximum buffer size for a HTTP request body.

  AZ_LOG_MSG_BUF_SIZE = 1024, ///< The maximum size of a log message.
};

#include <_az_cfg_suffix.h>

#endif // _az_CONFIG_H
