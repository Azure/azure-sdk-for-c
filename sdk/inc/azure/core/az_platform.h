// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_H
#define _az_PLATFORM_H

#include <azure/core/az_result.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

#if __has_include("az_platform_impl.h")
    //Users never directly include az_impl
    //......
    #include <az_platform_impl.h>
#else
    // Spit out error
    #error "Error go see the docs"

#endif //__has_include

#endif // _az_PLATFORM_H