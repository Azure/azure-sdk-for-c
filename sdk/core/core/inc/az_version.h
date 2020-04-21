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

int az_core_version_get_major();
int az_core_version_get_minor();
int az_core_version_get_patch();
const char* az_core_version_get_prerelease();

#endif //_az_VERSION_H
