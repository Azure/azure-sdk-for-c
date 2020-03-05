// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_config.h
 *
 * @brief configurable constants for SDK behavior
 */

#ifndef _az_CONFIG_H
#define _az_CONFIG_H

#include <_az_cfg_prefix.h>

/*
 *
 */
enum
{
  AZ_HTTP_REQUEST_URL_BUF_SIZE = 2 * 1024,
  AZ_HTTP_REQUEST_BODY_BUF_SIZE = 1024,

  AZ_LOG_MSG_BUF_SIZE = 1024,  // Size (in bytes) of the buffer to allocate on stack when building a
                               // log message => the maximum size of the log message.
};

#include <_az_cfg_suffix.h>

#endif // _az_CONFIG_H
