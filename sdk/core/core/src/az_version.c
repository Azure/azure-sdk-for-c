// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_version.h"

int az_core_version_get_major() { return AZ_VERSION_MAJOR; }
int az_core_version_get_minor() { return AZ_VERSION_MINOR; }
int az_core_version_get_patch() { return AZ_VERSION_PATCH; }
const char* az_core_version_get_prerelease() { return AZ_VERSION_PRERELEASE; }
