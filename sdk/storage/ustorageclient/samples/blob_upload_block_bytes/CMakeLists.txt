#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.2)

add_standards()

usePermissiveRulesForSamplesAndTests()

set(this_sample_name blob_upload_block_bytes)

#Add sample h files
set(${this_sample_name}_h_dir
    ${PROJECT_SOURCE_DIR}/samples/${this_sample_name}/inc
    ${PROJECT_SOURCE_DIR}/samples/${this_sample_name}/config
)

#Add sample c files
set(${this_sample_name}_c_files
    ./main.c
)

azstorage_build_c_sample_target(${this_sample_name} "samples/blob_upload_block_bytes")
