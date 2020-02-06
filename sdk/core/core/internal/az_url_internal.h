// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_URL_INTERNAL_H
#define _az_URL_INTERNAL_H

#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span userinfo;
  az_span host;
  az_span port;
} az_url_authority;

/**
 * https://en.wikipedia.org/wiki/URL
 * https://tools.ietf.org/html/rfc3986#section-3.2
 */
typedef struct {
  az_span scheme;
  az_url_authority authority;
  az_span path;
  az_span query;
  az_span fragment;
  struct {
    az_span reader;
  } _internal;
} az_url;

AZ_NODISCARD az_result az_url_parse(az_span url, az_url * const out);

/**
 * Read backwards (from right to left) from top-level domain (eg. `.com`) to the lowest subdomain
 * (eg. `www`).
 */
AZ_NODISCARD az_result az_host_read_domain(az_span * host, az_span * const domain);

#include <_az_cfg_suffix.h>

#endif
