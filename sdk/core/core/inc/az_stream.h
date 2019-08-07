// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_H
#define AZ_HTTP_H

#include "az_error.h"
#include "az_cstr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef az_error (*az_write)(void *read_context, az_cstr str);

typedef az_error (*az_read)(void *write_context, az_cstr *out_str);

#ifdef __cplusplus
}
#endif

#endif
