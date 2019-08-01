# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.7)

project(az_exampleshortname
    LANGUAGES C
    VERSION 0.1.0)

option(BUILD_DOCS "Build documentation" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(AZ_INSTALL_CMAKEPACKAGEDIR "share/cmake" CACHE STRING "directory to install cmake packages into")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)


file(GLOB_RECURSE az_exampleshortname_src CONFIGURE_DEPENDS src/*.c)
file(GLOB_RECURSE az_exampleshortname_example_src CONFIGURE_DEPENDS examples/*.c)

add_library(az_exampleshortname ${az_exampleshortname_src} exports.def)
get_filename_component(linker_name ${CMAKE_LINKER} NAME)
if(NOT linker_name MATCHES link)
    set_property(TARGET az_exampleshortname PROPERTY LINK_FLAGS -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.lds)
endif()
add_library(az::exampleshortname ALIAS az_exampleshortname)
target_include_directories(az_exampleshortname PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/inc>
    $<INSTALL_INTERFACE:include>)

if(BUILD_EXAMPLES) 
foreach(example_src IN LISTS az_exampleshortname_example_src)
    get_filename_component(example_name ${example_src} NAME_WE)
    add_executable(${example_name} ${example_src})
    target_link_libraries(${example_name} PRIVATE az_exampleshortname)
endforeach()
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
install(TARGETS az_exampleshortname EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY inc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ${PROJECT_NAME}-targets DESTINATION ${AZ_INSTALL_CMAKEPACKAGEDIR}/${PROJECT_NAME} NAMESPACE az::)
