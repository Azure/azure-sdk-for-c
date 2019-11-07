# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project(az LANGUAGES C)
enable_testing ()

# make options for each subdirectoy so it can be selected what to build on CI Pipelines
option(BUILD_AZ_CORE "Build lib and targets from Azure CORE" ON)
option(BUILD_AZ_KEY_VAULT_CLIENT "Build lib and targets from Azure KeyVault client" OFF)
option(BUILD_AZ_USTORAGE_CLIENT "Build targets from u-storage cliente" OFF)

if (BUILD_AZ_CORE)
  add_subdirectory(sdk/core/core)
endif()
if (BUILD_AZ_KEY_VAULT_CLIENT)
  # adding core only for linking library. No tests or executables will be created
  if (NOT BUILD_AZ_CORE)
    add_subdirectory(sdk/core/core)
  endif()
  add_subdirectory(sdk/keyvault/keyvault)
endif()
if (BUILD_AZ_USTORAGE_CLIENT)
  add_subdirectory(sdk/storage/ustorageclient)
endif()
