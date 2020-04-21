// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_version.h
 *
 * @brief Provides version information
 *
 * NOTE: You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_VERSION_H
#define _az_VERSION_H

// @brief AZ_SDK_VERSION_STRING Version string for telemetry
#define AZ_SDK_VERSION_STRING "1.0.0-preview.1"

// @brief AZ_SDK_VERSION_MAJOR Major portion of the Version
#define AZ_SDK_VERSION_MAJOR 1

// @brief AZ_SDK_VERSION_MINOR Minor portion of the Version
#define AZ_SDK_VERSION_MINOR 0

// @brief AZ_SDK_VERSION_PATCH Patch portion of the Version
#define AZ_SDK_VERSION_PATCH 0

// @brief AZ_SDK_VERSION_PRERELEASE Optional portion of the version
//  SDK is in a prerelease state when prerelease is present
//  See https://semver.org
#define AZ_SDK_VERSION_PRERELEASE "preview.1"

#endif //_az_VERSION_H
