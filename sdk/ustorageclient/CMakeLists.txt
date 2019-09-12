#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.2)

project(azure_iot_ustorageclient)

set(AZURE_IOT_USTORAGECLIENT_VERSION 1.0.1)

#Include helper functions
include("build_all/configs/ustorageclient_functions.cmake")
include("build_all/configs/ustorageclient_build_rules.cmake")

#Set the standards for C and C++
add_standards()

#Specify user options
option(run_azstorage_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF)" OFF)
option(run_azstorage_unit_tests "set run_unittests to ON to run unit tests (default is OFF)" OFF)
option(skip_azstorage_samples "set skip_samples to ON to skip building samples (default is OFF)[if possible, they are always built]" OFF)
option(use_azstorage_installed_dependencies "set use_installed_dependencies to ON to use installed packages instead of building dependencies from submodules" OFF)

#Set ustorageclient c files
set(azure_iot_ustorageclient_c_files
    ${PROJECT_SOURCE_DIR}/src/azstorage_blob.c
)

#Add library of ustorageclient c files
add_library(azure_iot_ustorageclient ${azure_iot_ustorageclient_c_files})

#Set the include directories for this project
set(azure_iot_ustorageclient_h_dir
    ${PROJECT_SOURCE_DIR}/inc
    ${PROJECT_SOURCE_DIR}/inc/_internal
    ${PROJECT_SOURCE_DIR}/config
)

#Add include directories for this target and anyone linking against it
target_include_directories(azure_iot_ustorageclient PUBLIC ${azure_iot_ustorageclient_h_dir})
target_include_directories(azure_iot_ustorageclient PRIVATE
    ${PROJECT_SOURCE_DIR}/deps/umock-c/inc
    ${PROJECT_SOURCE_DIR}/deps/azure-macro-utils-c/inc
    ${PROJECT_SOURCE_DIR}/deps/azure-c-shared-utility/inc
    #${PROJECT_SOURCE_DIR}/deps/azure-ulib-c/inc
    #${PROJECT_SOURCE_DIR}/deps/azure-ulib-c/config
    #ulib pal directory TODO: fix with environment variable from azure-ulib-c cmake
    #${PROJECT_SOURCE_DIR}/deps/azure-ulib-c/pal/MSBUILD/X86
)

set(AZURE_IOT_USTORAGECLIENT_INC_FOLDER ${PROJECT_SOURCE_DIR}/inc CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)

add_subdirectory(${PROJECT_SOURCE_DIR}/deps/umock-c EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-ctest EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-macro-utils-c EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-c-testrunnerswitcher EXCLUDE_FROM_ALL)
#turn on use_built_in_httpapi option for azure-c-shared-utility
set(use_builtin_httpapi ON CACHE BOOL "enable built-in httpapi_compact")
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-c-shared-utility EXCLUDE_FROM_ALL)
#add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-ulib-c EXCLUDE_FROM_ALL)

setTargetBuildProperties(azure_iot_ustorageclient)

if (${run_azstorage_unit_tests})
    add_subdirectory(tests)
endif()
if (${run_azstorage_e2e_tests})
    add_subdirectory(e2e_tests)
endif()
if (NOT ${skip_azstorage_samples})
    add_subdirectory(samples)
endif()
