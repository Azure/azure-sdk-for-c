// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CALLBACK_H
#define AZ_CALLBACK_H

#include <_az_cfg_prefix.h>

typedef void * az_callback_data;

#define AZ_CAT(A, B) A##B

#define AZ_CALLBACK_ARG(NAME) AZ_CAT(NAME, _arg)

#define AZ_CALLBACK_DECL(NAME, ARG) \
  typedef ARG AZ_CALLBACK_ARG(NAME); \
  typedef struct { \
    az_result (*func)(az_callback_data const, ARG const); \
    az_callback_data data; \
  } NAME;

#define AZ_CALLBACK_DATA(NAME, DATA, CALLBACK) \
  AZ_STATIC_ASSERT(sizeof(DATA) <= sizeof(az_callback_data)) \
  AZ_INLINE CALLBACK NAME( \
      DATA const data, az_result (*const func)(DATA const, AZ_CALLBACK_ARG(CALLBACK) const)) { \
    return (CALLBACK){ \
      .func = (az_result(*)(az_callback_data, AZ_CALLBACK_ARG(CALLBACK)))func, \
      .data = (az_callback_data)data, \
    }; \
  }

#include <_az_cfg_suffix.h>

#endif
