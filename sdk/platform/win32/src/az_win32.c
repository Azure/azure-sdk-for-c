// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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

#include <az_platform_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }
