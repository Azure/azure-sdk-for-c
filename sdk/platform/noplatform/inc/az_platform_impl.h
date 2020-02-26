// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

#include <_az_cfg_prefix.h>

struct az_platform_mtx
{
  /// @internal
  struct
  {
    char unused;
  } _internal;
  /// @endinternal
};

#include <_az_cfg_suffix.h>

#endif // _az_PLATFORM_IMPL_H
