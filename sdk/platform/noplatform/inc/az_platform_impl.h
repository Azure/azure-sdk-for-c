// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

#include <_az_cfg_prefix.h>

/**
 * @brief A platform specific abstraction for a mutex.
 *
 */
struct az_platform_mtx
{
  struct
  {
    // We can't have an empty struct because C
    // requires that a struct or union have at least one member.
    char unused;
  } _internal;
};

#include <_az_cfg_suffix.h>

#endif // _az_PLATFORM_IMPL_H
