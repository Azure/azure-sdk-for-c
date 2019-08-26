// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**
 *  @file _internal.h
 *  @brief Internal framework for UStorageClient APIs
*/

#ifndef _AZSTORAGE_INTERNAL_H
#define _AZSTORAGE_INTERNAL_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C"
{
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/httpapi.h"

/**
 * @brief   define offset_t as type size_t
 */
typedef size_t     offset_t;

/**
 * @brief   Storage client containing virtual table to internal functions
 * @warning DO NOT USE OR TRY TO ACCESS THIS DATA STRUCTURE DIRECTLY!
 *          Use AZSTORAGE_BLOB instead.
 */
typedef struct AZSTORAGE_BLOB_STORAGE_CLIENT_TAG
{
    // connection details
    const char* service_uri;

    // connection options
    HTTP_HANDLE http_handle;
} AZSTORAGE_BLOB_STORAGE_CLIENT;

/**
 * @brief blob type strings to be used when creating a blob
 */
#define AZSTORAGE_BLOB_TYPE_BLOCK_BLOB_STRING             "BlockBlob"
#define AZSTORAGE_BLOB_TYPE_APPEND_BLOB_STRING            "AppendBlob"


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AZSTORAGE_INTERNAL_H */