// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_H
#define _az_PLATFORM_H

#include <windows.h>

#include <_az_cfg_prefix.h>

struct az_platform_mtx {
  CRITICAL_SECTION cs;
};

#include <_az_cfg_suffix.h>

#endif
