// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Provides version information.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_VERSION_H
#define _az_VERSION_H

#define _az_STRINGIFY2(x) #x
#define _az_STRINGIFY(x) _az_STRINGIFY2(x)

/// Major numeric identifier.
#define AZ_SDK_VERSION_MAJOR 1

/// Minor numeric identifier.
#define AZ_SDK_VERSION_MINOR 2

/// Patch numeric identifier.
#define AZ_SDK_VERSION_PATCH 0

/// Optional pre-release identifier. SDK is in a pre-release state when present.
#define AZ_SDK_VERSION_PRERELEASE "beta.1"

/// The version in string format used for telemetry following the `semver.org` standard
/// (https://semver.org).
#define AZ_SDK_VERSION_STRING \
    _az_STRINGIFY( AZ_SDK_VERSION_MAJOR ) "." \
    _az_STRINGIFY( AZ_SDK_VERSION_MINOR ) "." \
    _az_STRINGIFY( AZ_SDK_VERSION_PATCH ) "-" \
    AZ_SDK_VERSION_PRERELEASE

#endif //_az_VERSION_H
