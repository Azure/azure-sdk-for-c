# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

include(CMakeDependentOption)
option(az_exampleshortname_allow_embed "allow embedding of libraries" OFF)
option(az_exampleshortname_embed_default "embed libraries by default" ON)
cmake_dependent_option(az_exampleshortname_embed_catch2 "use embedded catch2" ${az_exampleshortname_embed_default} 
    "az_exampleshortname_allow_embed" OFF)

# this makes set() override option() even in projects
# that specify a minimum_required version before that policy
# was introduced
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if(az_exampleshortname_embed_catch2)
    set(CATCH_BUILD_TESTING OFF)
    set(CATCH_INSTALL_DOCS OFF)
    set(CATCH_INSTALL_HELPERS OFF)
    # EXCLUDE_FROM_ALL means doctest won't be built unless some target links to a library from doctest.
    add_subdirectory(Catch2 EXCLUDE_FROM_ALL)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/contrib)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
endif()
