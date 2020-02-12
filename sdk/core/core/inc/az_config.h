// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CONFIG_H
#define _az_CONFIG_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

/*
 *
 */
enum {
  AZ_HTTP_REQUEST_URL_BUF_SIZE = 2* 1024,
  AZ_HTTP_REQUEST_BODY_BUF_SIZE = 1024,
  AZ_HTTP_REQUEST_HEADER_COUNT = 10,
  AZ_HTTP_REQUEST_HEADER_BUF_SIZE = AZ_HTTP_REQUEST_HEADER_COUNT * sizeof(az_pair),
  AZ_HTTP_RESPONSE_BUF_SIZE = 3 * 1024
};

#include <_az_cfg_suffix.h>

#endif /* _az_CONFIG_H */
