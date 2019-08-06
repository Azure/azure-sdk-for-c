# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.7)

project(az_exampleshortname
    LANGUAGES C
    VERSION 0.1.0)

option(BUILD_DOCS "Build documentation" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(CMAKE_C_STANDARD 99)

file(GLOB_RECURSE az_exampleshortname_src CONFIGURE_DEPENDS src/*.c)
file(GLOB_RECURSE az_exampleshortname_example_src CONFIGURE_DEPENDS examples/*.c)

add_library(az_exampleshortname)
# add an alias so that we can be consumed using add_subdirectory or
# find_package with the same targets. See: https://cliutils.gitlab.io/modern-cmake/chapters/install.html
add_library(az::exampleshortname ALIAS az_exampleshortname)

# add sources to our target
target_sources(az_exampleshortname PRIVATE ${az_exampleshortname_src})

# if there are include files, we tell cmake (and thus our compiler) about them here.
# in general include directories should be kept in /inc inside a project, in the same structure
# as they would appear if installed in /usr/include. Thus we add /inc to the include paths of
# targets in the same build tree as our example project, and include to the path of targets
# using an installed version of our library.
#
# Note: cmake will add the install prefix when you call find_package, so just include works.
target_include_directories(az_exampleshortname PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>)

# BEGIN: things you need for building shared libraries
# add a def file for exporting on windows, this does no harm
# on other platforms (it'll be a nop)
target_sources(az_exampleshortname PRIVATE exports.def)

# If we suspect we're not linking with either microsoft LINK.EXE or
# another linker pretending to be LINK.exe (like lld-link) then try and add a version script.
get_filename_component(linker_name ${CMAKE_LINKER} NAME)
if(NOT linker_name MATCHES link)
    set_property(TARGET az_exampleshortname PROPERTY LINK_FLAGS -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.lds)
endif()
#END: things you need for building shared libraries

#BEGIN: things you need to build examples
if(BUILD_EXAMPLES) 
foreach(example_src IN LISTS az_exampleshortname_example_src)
    get_filename_component(example_name ${example_src} NAME_WE)
    add_executable(${example_name} ${example_src})
    target_link_libraries(${example_name} PRIVATE az_exampleshortname)
endforeach()
endif()
#END: things you need to build examples

#BEGIN: things you need to build documentation
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
        ${PROJECT_SOURCE_DIR}/doc
        ${PROJECT_SOURCE_DIR}/src
        COMMENT "Generate Documentation")
endif()
#END: Things you need to build documentation

#BEGIN: things you need to generate install targets
set(AZ_INSTALL_CMAKEPACKAGEDIR "share/cmake" CACHE STRING "directory to install cmake packages into")
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
#END: Things you need to generate install targets