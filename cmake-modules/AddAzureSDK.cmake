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
FetchContent_Declare(cppstoragesdk
    GIT_REPOSITORY      https://github.com/Azure/azure-sdk-for-cpp.git
    GIT_TAG             azure-template_1.0.0-beta.3)
FetchContent_GetProperties(cppstoragesdk)
if(NOT cppstoragesdk_POPULATED)
    FetchContent_Populate(cppstoragesdk)
    add_subdirectory(${cppstoragesdk_SOURCE_DIR} ${cppstoragesdk_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
