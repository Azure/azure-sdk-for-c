// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STREAM_H
#define AZ_STREAM_H

#include "az_error.h"
#include "az_cstr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef az_error (*az_write)(void *read_context, az_cstr str);

typedef struct {
  void *context;
  az_write func;
} az_write_closure;

// `out_str` is empty if stream is empty.
typedef az_error (*az_read)(void *write_context, az_cstr *out_str);

typedef struct {
  void *context;
  az_read func;
} az_read_closure;

#ifdef __cplusplus
}
#endif

#endif
