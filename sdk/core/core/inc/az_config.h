// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CONFIG_H
#define _az_CONFIG_H

#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/*
* 
*/
enum { 
    AZ_HTTP_URL_MAX_SIZE = 1024 * 2, 
    AZ_HTTP_MAX_BODY_SIZE = 1024 
};

#include <_az_cfg_suffix.h>

#endif /* _az_CONFIG_H */