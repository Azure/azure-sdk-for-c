// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef AZSTORAGE_STORAGE_CONFIG_H
#define AZSTORAGE_STORAGE_CONFIG_H

/**
 * @brief  definition to trigger argument tests
 *
 * OPTION: Comment this out if you do not want to run tests on arguments you are passing into
 * the APIs and would like your program to run faster.
 * @warning: errors due to improper use of arguments will propagate down the
 * call stack and will be hard to interpret.
 * Only comment this out if you have already tested your function arguments.
 */
// TODO: once azure-ulib-c is public this can be enabled
// #define AZSTORAGE_BLOB_VALIDATE_ARGUMENTS

/**
 * @brief bytes allocated to storing a whole blob in an array
 */
#define AZSTORAGE_AVAILABLE_STORAGE                 1024

/**
 * @brief bytes allocated to buffer stream
 */
#define AZSTORAGE_BUFFER_SIZE                       512

/**
 * @brief bytes allocated to blob size string
 * @note  14 based on the 8TB blob limit
 */
#define AZSTORAGE_SIZE_STR_SIZE                     14

/**
 * @brief bytes allocated to byte range header for blob get
 * @note  35 based on the 8TB blob limit
 */
#define AZSTORAGE_BYTERANGE_HEADER_SIZE             35

#endif /* AZSTORAGE_STORAGE_CONFIG_H */
