# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)

project(az_exampleshortname
    LANGUAGES C CXX
    VERSION 0.1.0)

option(az_exampleshortname_with_docs "Build documentation" ON)
option(BUILD_TESTING "Build tests" ON)





# this is a cmake 3.15 feature that tells find_package to 
# look at config files before module files. This makes it easier
# to write buildsystems that work on many platforms, espicially
# when finding libraries that have multiple build systems.


# if needed: add ./cmake to the module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# default to hidden symbols when building shared libraries
# on unix systems
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set_property(DIRECTORY PROPERTY COMPILE_OPTIONS
    $<$<COMPILE_LANG_AND_ID:C,MSVC>:/W4 /wd4201>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Xclang -Wall -Wextra -Wno-microsoft-anon-tag -Wdocumentation>)

add_subdirectory(external)

if(NOT DEFINED ECM_MODULE_PATH)
    find_package(ECM REQUIRED CONFIG)
endif()
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})


file(GLOB_RECURSE az_exampleshortname_src CONFIGURE_DEPENDS src/*.c)
file(GLOB_RECURSE az_exampleshortname_test_src CONFIGURE_DEPENDS test/*.cpp)
add_library(az_exampleshortname)
add_library(az::exampleshortname ALIAS az_exampleshortname)
include(GenerateExportHeader)
generate_export_header(az_exampleshortname 
    EXPORT_FILE_NAME inc/az/exampleshortname/export.h)
target_sources(az_exampleshortname PRIVATE ${az_exampleshortname_src})
target_include_directories(az_exampleshortname PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/inc>
    $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/az/exampleshortname
        ${CMAKE_CURRENT_BINARY_DIR}/inc/az/exampleshortname)
if(BUILD_TESTING)
    include(CTest)
    if(NOT TARGET Catch2::Catch2)
        find_package(Catch2 REQUIRED CONFIG)
    endif()
    include(Catch)
    add_executable(az_exampleshortname_tests)
    target_sources(az_exampleshortname_tests PRIVATE ${az_exampleshortname_test_src})
    target_link_libraries(az_exampleshortname_tests PRIVATE az_exampleshortname Catch2::Catch2)
endif()


# generate the install steps
include(KDEInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(config.cmake.in ${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_CMAKEPACKAGEDIR}/${PROJECT_NAME})
install(TARGETS az_exampleshortname EXPORT ${PROJECT_NAME}-targets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(DIRECTORY inc DESTINATION ${KDE_INSTALL_INCLUDEDIR})
install(EXPORT ${PROJECT_NAME}-targets DESTINATION ${KDE_INSTALL_CMAKEPACKAGEDIR}/${PROJECT_NAME} NAMESPACE az::)