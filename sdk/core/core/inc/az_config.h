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
  AZ_HTTP_REQUEST_HEADER_COUNT = 10,
};

#include <_az_cfg_suffix.h>

#endif // _az_CONFIG_H
