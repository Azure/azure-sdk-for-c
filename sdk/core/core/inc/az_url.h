// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_URL_H
#define AZ_URL_H

#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span userinfo;
  az_span host;
  az_span port;
} az_url_authority;

typedef struct {
  az_span authority;
  az_span path;
} az_url_hier_part;

/**
 * https://en.wikipedia.org/wiki/URL
 * https://tools.ietf.org/html/rfc3986#section-3.2
 */
typedef struct {
  az_span scheme;
  az_url_hier_part hier_part;
  az_span query;
  az_span fragment;
} az_url;

AZ_NODISCARD az_result az_url_parse(az_span const url, az_url * const out);

/**
 * Read backwards (from right to left) from top-level domain (eg. `.com`) to the lowest subdomain
 * (eg. `www`).
 */
AZ_NODISCARD az_result az_dns_read_domain(az_span * const dns, az_span * const subdomain);

#include <_az_cfg_suffix.h>

#endif
