# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT
#
# How to use: Copy this file to a cmake modules folder within your project.
# - Add `list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/your-cmake-modules-folder")`
# - Call this module by doing `include(AddAzureSDK)` from root CMakeLists.txt
# - Link your application against your cmake targets like `target_link_libraries(yourApp PRIVATE az_core)`
#
# This will create a _deps folder for the generated project and Azure SDK will be checked out in there.
# When calling `cmake --build .`, Azure SDK will be built before your application and become available.
#
# Cmake options for Azure SDK for C can be set together with your application cmake options.
#
# Note: Update GIT_TAG to the expected version from Azure SDK for C
#

include(FetchContent)
FetchContent_Declare(azuresdkforc
    GIT_REPOSITORY      https://github.com/Azure/azure-sdk-for-c.git
    GIT_TAG             1.0.0-preview.5)
FetchContent_GetProperties(azuresdkforc)
if(NOT azuresdkforc_POPULATED)
    FetchContent_Populate(azuresdkforc)
    add_subdirectory(${azuresdkforc_SOURCE_DIR} ${azuresdkforc_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
