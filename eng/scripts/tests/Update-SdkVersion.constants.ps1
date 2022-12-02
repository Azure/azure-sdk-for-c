$AzVersionLocation = "$PSScriptRoot../../../../sdk/inc/azure/core/az_version.h"
$ChangelogLocation = "$PSScriptRoot../../../../CHANGELOG.md"
$AzVersionContentBeta = @"
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

/// The version in string format used for telemetry following the `semver.org` standard
/// (https://semver.org).
#define AZ_SDK_VERSION_STRING "1.0.0-beta.1"

/// Major numeric identifier.
#define AZ_SDK_VERSION_MAJOR 1

/// Minor numeric identifier.
#define AZ_SDK_VERSION_MINOR 8

/// Patch numeric identifier.
#define AZ_SDK_VERSION_PATCH 0

/// Optional pre-release identifier. SDK is in a pre-release state when present.
#define AZ_SDK_VERSION_PRERELEASE "beta.1"

#endif //_az_VERSION_H
"@

$AzVersionContentGa = @"
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

/// The version in string format used for telemetry following the `semver.org` standard
/// (https://semver.org).
#define AZ_SDK_VERSION_STRING "1.0.0"

/// Major numeric identifier.
#define AZ_SDK_VERSION_MAJOR 1

/// Minor numeric identifier.
#define AZ_SDK_VERSION_MINOR 0

/// Patch numeric identifier.
#define AZ_SDK_VERSION_PATCH 0

/// Optional pre-release identifier. SDK is in a pre-release state when present.
#define AZ_SDK_VERSION_PRERELEASE ""
#undef AZ_SDK_VERSION_PRERELEASE

#endif //_az_VERSION_H
"@

function CheckoutChangedFiles {
    git checkout $AzVersionLocation
    git checkout $ChangelogLocation
}