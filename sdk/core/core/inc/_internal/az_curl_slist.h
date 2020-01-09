// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CURL_SLIST_H
#define _az_CURL_SLIST_H

#include <az_pair.h>
#include <az_result.h>

#include <curl/curl.h>

#include <_az_cfg_prefix.h>

/**
 * Appends a zero-terminated string to the given CURL list.
 */
AZ_NODISCARD az_result
az_curl_slist_append(struct curl_slist ** const self, char const * const str);

/**
 * Appends an HTTP header to the given CURL list.
 */
AZ_NODISCARD az_result
az_curl_slist_append_header(struct curl_slist ** const pp_list, az_pair const header);

#include <_az_cfg_suffix.h>

#endif
