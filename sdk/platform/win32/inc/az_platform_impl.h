// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

// Include windows.h with WIN32_LEAN_AND_MEAN and NOMINMAX defined, but restore the ifdef state
// afterwards.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#else // !NOMINMAX
#include <windows.h>
#endif // NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#else // !WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#else // !NOMINMAX
#include <windows.h>
#endif // NOMINMAX
#endif // WIN32_LEAN_AND_MEAN

#include <_az_cfg_prefix.h>

/**
 * @brief A platform specific abstraction for a mutex.
 *
 */
struct az_platform_mtx
{
  struct
  {
    CRITICAL_SECTION cs;
  } _internal;
};

#include <_az_cfg_suffix.h>

#endif // _az_PLATFORM_IMPL_H
