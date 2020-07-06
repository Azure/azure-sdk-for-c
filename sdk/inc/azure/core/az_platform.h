// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_H
#define _az_PLATFORM_H

#include <azure/core/az_result.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

#if __has_include("az_platform_impl.h")
    #include <az_platform_impl.h>
#else
    // A platform implementation is required when including az_platform.h.
    // The Azure SDK for Embedded C provides default implementations for Win32, Linux and Mac.
    // Developers can provide other platform implementations as well.
    //  https://github.com/Azure/azure-sdk-for-c#cmake-options
    #error "No platform implementation provided, see documentation for additional details.  https://github.com/Azure/azure-sdk-for-c#cmake-options"

#endif //__has_include

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_H
