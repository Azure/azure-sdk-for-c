// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>

// Two macros below are not used in the code below, it is windows.h that consumes them.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }
