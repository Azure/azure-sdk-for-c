// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_FUNCTOR_H
#define AZ_FUNCTOR_H

#include <_az_cfg_prefix.h>

typedef void * az_functor_data;

#define AZ_CAT(A, B) A##B

#define AZ_FUNCTOR_ARG(NAME) AZ_CAT(NAME, _arg)

#define AZ_FUNCTOR_DECL(NAME, ARG) \
  typedef ARG AZ_FUNCTOR_ARG(NAME); \
  typedef struct { \
    az_result (*func)(az_functor_data const, ARG const); \
    az_functor_data data; \
  } NAME;

#define AZ_FUNCTOR_DATA(NAME, DATA, FUNCTOR) \
  AZ_STATIC_ASSERT(sizeof(DATA) <= sizeof(az_functor_data)) \
  AZ_INLINE FUNCTOR NAME( \
      DATA const data, az_result (*const func)(DATA const, AZ_FUNCTOR_ARG(FUNCTOR) const)) { \
    return (FUNCTOR){ \
      .func = (az_result(*)(az_functor_data, AZ_FUNCTOR_ARG(FUNCTOR)))func, \
      .data = (az_functor_data)data, \
    }; \
  }

#include <_az_cfg_suffix.h>

#endif
