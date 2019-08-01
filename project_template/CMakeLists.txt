# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)

project(az_exampleshortname
    LANGUAGES C CXX
    VERSION 0.1.0)

option(BUILD_DOCS "Build documentation" ON)
option(BUILD_TESTING "Build tests" ON)
set(AZ_INSTALL_CMAKEPACKAGEDIR "share/cmake" CACHE STRING "directory to install cmake packages into")

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


file(GLOB_RECURSE az_exampleshortname_src CONFIGURE_DEPENDS src/*.c)
file(GLOB_RECURSE az_exampleshortname_test_src CONFIGURE_DEPENDS test/*.cpp)
add_library(az_exampleshortname)
include(GenerateExportHeader)
generate_export_header(az_exampleshortname EXPORT_FILE_NAME inc/az/exampleshortname/export.h)
add_library(az::exampleshortname ALIAS az_exampleshortname)
target_sources(az_exampleshortname PRIVATE ${az_exampleshortname_src})
target_include_directories(az_exampleshortname PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/inc>
    $<INSTALL_INTERFACE:include>)

if(BUILD_TESTING)
    enable_testing()
    if(NOT TARGET Catch2::Catch2)
        find_package(Catch2 REQUIRED CONFIG)
    endif()
    include(Catch)
    add_executable(az_exampleshortname_tests)
    target_sources(az_exampleshortname_tests PRIVATE ${az_exampleshortname_test_src})
    target_link_libraries(az_exampleshortname_tests PRIVATE az_exampleshortname Catch2::Catch2)
    catch_discover_tests(az_exampleshortname_tests)
endif()

#generate documentation
if(BUILD_DOCS)
    if(NOT TARGET Doxygen::doxygen)
        find_package(Doxygen REQUIRED doxygen)
    endif()

    # note: DOXYGEN_ options are strings to cmake, not
    # booleans, thus use only YES and NO
    set(DOXYGEN_GENERATE_XML YES)
    set(DOXYGEN_GENERATE_HTML YES)

    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_EXTRACT_PACKAGE YES)
    set(DOXYGEN_INLINE_SIMPLE_STRUCTS YES)
    set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
    doxygen_add_docs(docs
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_BINARY_DIR}/inc
        ${PROJECT_SOURCE_DIR}/doc
        ${PROJECT_SOURCE_DIR}/src
        COMMENT "Generate Documentation")
endif()

# generate the install steps
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(config.cmake.in ${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${AZ_INSTALL_CMAKEPACKAGEDIR}/${PROJECT_NAME})
install(TARGETS az_exampleshortname EXPORT ${PROJECT_NAME}-targets)
install(DIRECTORY inc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ${PROJECT_NAME}-targets DESTINATION ${AZ_INSTALL_CMAKEPACKAGEDIR}/${PROJECT_NAME} NAMESPACE az::)
