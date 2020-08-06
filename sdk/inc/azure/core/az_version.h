// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_version.h
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

// @brief Version in string format for telemetry.
//   Follows semver.org standard.
//   https://semver.org
#define AZ_SDK_VERSION_STRING "1.0.0-preview.4"

// @brief Major numeric identifier.
#define AZ_SDK_VERSION_MAJOR 1

// @brief Minor numeric identifier.
#define AZ_SDK_VERSION_MINOR 0

// @brief Patch numeric identifier.
#define AZ_SDK_VERSION_PATCH 0

// @brief Optional pre-release identifier.
//  SDK is in a prerelease state when present.
#define AZ_SDK_VERSION_PRERELEASE "preview.4"

#endif //_az_VERSION_H
