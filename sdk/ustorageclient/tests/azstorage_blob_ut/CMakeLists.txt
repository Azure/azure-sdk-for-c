#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(thisTestSuiteName azstorage_blob_ut)

set(${thisTestSuiteName}_test_files
    ${thisTestSuiteName}.c
    ${PROJECT_SOURCE_DIR}/tests/${thisTestSuiteName}/main.c
)

set(${thisTestSuiteName}_c_files
    ${PROJECT_SOURCE_DIR}/src/azstorage_blob.c
)

set(${thisTestSuiteName}_h_files
    ${PROJECT_SOURCE_DIR}/inc/azstorage_blob_api.h
)
include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/config)
include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})
include_directories(
    ${PROJECT_SOURCE_DIR}/deps/azure-c-shared-utility/inc
    ${PROJECT_SOURCE_DIR}/deps/umock-c/inc
    ${PROJECT_SOURCE_DIR}/deps/azure-macro-utils-c/inc
    ${PROJECT_SOURCE_DIR}/deps/azure-c-shared-utility/inc
    #${PROJECT_SOURCE_DIR}/deps/azure-ulib-c/inc
    #${PROJECT_SOURCE_DIR}/deps/azure-ulib-c/config
    #ulib pal directory TODO: fix with environment variable from azure-ulib-c cmake
    #${PROJECT_SOURCE_DIR}/deps/azure-ulib-c/pal/MSBUILD/X86
)

add_definitions(-DNO_LOGGING)

azstorage_build_c_test_target(${thisTestSuiteName} "tests/azstorage_blob_tests")

# compile_c_test_artifacts_as(${thisTestSuiteName} C99)